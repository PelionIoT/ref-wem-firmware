// ****************************************************************************
//  Firmware Over The Air (FOTA) demo
//
//  This application demonstrates how to perform fota using mbed cloud 1.2.
//
//  By the ARM Reference Design (Red) Team
// ****************************************************************************
#include "m2mclient.h"

#include "GL5528.h"

#include "ledman.h"

#include <errno.h>
#include <factory_configurator_client.h>
#include <mbed-trace-helper.h>
#include <mbed-trace/mbed_trace.h>
#include <SDBlockDevice.h>
#include <TextLCD.h>

#if MBED_CONF_APP_WIFI
    #include <ESP8266Interface.h>
#else
    #include <EthernetInterface.h>
#endif

// ****************************************************************************
// DEFINEs and type definitions
// ****************************************************************************
enum FOTA_THREADS {
    FOTA_THREAD_LED = 0,
    FOTA_THREAD_SENSOR_LIGHT,
    FOTA_THREAD_COUNT
};

// ****************************************************************************
// Globals
// ****************************************************************************
/* declared in pal_plat_fileSystem.cpp, which is included because COMMON_PAL
 * is defined in mbed_app.json */
extern SDBlockDevice sd;

Thread tman[FOTA_THREAD_COUNT] = {
    /* LEDs */
    { osPriorityNormal }
};

class MultiAddrLCD
{
public:
    MultiAddrLCD(I2C *i2c) : _lcd1(i2c, 0x4e, TextLCD::LCD16x2, TextLCD::HD44780),
                             _lcd2(i2c, 0x7e, TextLCD::LCD16x2, TextLCD::HD44780) {
    }

    /*Only supporting 16x2 LCDs, so string will be truncated at 32
      characters.*/
    int printf(const char* format, ...) {
        int rc;
        char buf[33];
        va_list args;
        va_start(args, format);
        vsnprintf(buf, 32, format, args);
        va_end(args);
        _lcd1.printf(buf);
        rc = _lcd2.printf(buf);
        return rc;
    }

    void setBacklight(TextLCD_Base::LCDBacklight mode) {
        _lcd1.setBacklight(mode);
        _lcd2.setBacklight(mode);
    }

    void setCursor(TextLCD_Base::LCDCursor mode) {
        _lcd1.setCursor(mode);
        _lcd2.setCursor(mode);
    }

private:
    TextLCD_I2C _lcd1;
    TextLCD_I2C _lcd2;
};

// ****************************************************************************
// Functions
// ****************************************************************************
static void thread_light_sensor()
{
    using namespace fota::sensor;
    light::LightSensor<light::BOARD_GROVE_GL5528> light(A0);

    while (true) {
        light.update();
        printf("reading: %2.2f\r\n", light.getFlux());
        wait(0.5f);
    }
}

static void thread_led_update()
{
    while (true) {
        led_post();
        wait(0.1f);
    }
}

static int platform_init()
{
    int ret;

#if MBED_CONF_MBED_TRACE_ENABLE
    /* Create mutex for tracing to avoid broken lines in logs */
    if (!mbed_trace_helper_create_mutex()) {
        printf("ERROR - Mutex creation for mbed_trace failed!\n");
        return -EACCES;
    }

    /* Initialize mbed trace */
    mbed_trace_init();
    mbed_trace_mutex_wait_function_set(mbed_trace_helper_mutex_wait);
    mbed_trace_mutex_release_function_set(mbed_trace_helper_mutex_release);
#endif

    /* init the sd card */
    ret = sd.init();
    if (ret != BD_ERROR_OK) {
        printf("sd init failed: %d\n", ret);
        return ret;
    }
    printf("sd init OK\n");

    /* setup the leds */
    led_setup();

    tman[FOTA_THREAD_LED].start(thread_led_update);
    tman[FOTA_THREAD_SENSOR_LIGHT].start(thread_light_sensor);
    return 0;
}

static void platform_shutdown()
{
    tman[FOTA_THREAD_LED].join();
}

static void mbed_client_on_registered(void *context)
{
    printf("mbed client registered\n");
    led_set_color(IND_CLOUD, IND_COLOR_SUCCESS);
}

static void mbed_client_on_unregistered(void *context)
{
    printf("mbed client unregistered\n");
    led_set_color(IND_CLOUD, IND_COLOR_OFF);
}

static void mbed_client_on_error(void *context)
{
    printf("mbed client ERROR\n");
    led_set_color(IND_CLOUD, IND_COLOR_FAILED);
}

static int run_mbed_client(NetworkInterface *iface)
{
    M2MClient mbed_client;

    mbed_client.create_resources();
    mbed_client.on_registered(NULL, mbed_client_on_registered);
    mbed_client.on_unregistered(NULL, mbed_client_on_unregistered);
    mbed_client.on_error(NULL, mbed_client_on_error);

    printf("mbed client: connecting\n");
    led_set_color(IND_CLOUD, IND_COLOR_IN_PROGRESS, true);
    mbed_client.call_register(iface);

    printf("mbed client: entering run loop\n");
    while (mbed_client.is_register_called()) {
        Thread::wait(1000);
    }
    printf("mbed client: exited run loop\n");

    return 0;
}

static int init_fcc(void)
{
    fcc_status_e ret;

#if MBED_CONF_APP_FCC_WIPE
    ret = fcc_storage_delete();
    if (ret != FCC_STATUS_SUCCESS) {
        printf("fcc: delete failed: %d\n", ret);
    }
#endif

    ret = fcc_init();
    if (ret != FCC_STATUS_SUCCESS) {
        printf("fcc: init failed: %d\n", ret);
        return ret;
    }

    ret = fcc_developer_flow();
    if (ret == FCC_STATUS_KCM_FILE_EXIST_ERROR) {
        printf("fcc: developer credentials already exists\n");
    } else if (ret != FCC_STATUS_SUCCESS) {
        printf("fcc: failed to load developer credentials\n");
        return ret;
    }

    ret = fcc_verify_device_configured_4mbed_cloud();
    if (ret != FCC_STATUS_SUCCESS) {
        printf("fcc: device not configured for mbed cloud\n");
        return ret;
    }

    return 0;
}

#if MBED_CONF_APP_WIFI
static nsapi_security_t wifi_security_str2sec(const char *security)
{
    if (0 == strcmp("WPA/WPA2", security)) {
        return NSAPI_SECURITY_WPA_WPA2;

    } else if (0 == strcmp("WPA2", security)) {
        return NSAPI_SECURITY_WPA2;

    } else if (0 == strcmp("WPA", security)) {
        return NSAPI_SECURITY_WPA;

    } else if (0 == strcmp("WEP", security)) {
        return NSAPI_SECURITY_WEP;

    } else if (0 == strcmp("NONE", security)) {
        return NSAPI_SECURITY_NONE;

    } else if (0 == strcmp("OPEN", security)) {
        return NSAPI_SECURITY_NONE;
    }

    printf("warning: unknown wifi security type (%s), assuming NONE\n",
           security);
    return NSAPI_SECURITY_NONE;
}

/**
 * brings up wifi
 * */
static NetworkInterface *init_network(void)
{
    int ret;

    ESP8266Interface *net;

    net = NULL;

    net = new ESP8266Interface(MBED_CONF_APP_WIFI_TX,
                               MBED_CONF_APP_WIFI_RX,
                               MBED_CONF_APP_WIFI_DEBUG);

    printf("[WIFI] connecting to: %s\n", MBED_CONF_APP_WIFI_SSID);
    ret = net->connect(MBED_CONF_APP_WIFI_SSID,
                       MBED_CONF_APP_WIFI_PASSWORD,
                       wifi_security_str2sec(MBED_CONF_APP_WIFI_SECURITY));
    if (0 != ret) {
        printf("[WIFI] Failed to connect to: %s (%d)\n",
               MBED_CONF_APP_WIFI_SSID, ret);
        delete net;
        return NULL;
    }
    printf("[WIFI] connected: ssid=%s, ip=%s, netmask=%s, gateway=%s\n",
           MBED_CONF_APP_WIFI_SSID,
           net->get_ip_address(),
           net->get_netmask(),
           net->get_gateway());

    return net;
}
#else
/**
 * brings up Ethernet
 * */
static NetworkInterface *init_network(void)
{
    int ret;

    EthernetInterface *net;

    net = NULL;

    net = new EthernetInterface();

    printf("[ETH] obtaining IP adress\n");
    ret = net->connect();
    if (0 != ret) {
        printf("[ETH] Failed to connect! %d\n", ret);
        delete net;
        return NULL;
    }
    printf("[ETH] connected: ip=%s, netmask=%s, gateway=%s\n",
           net->get_ip_address(), net->get_netmask(), net->get_gateway());

    return net;
}
#endif

// main() runs in its own thread in the OS
int main()
{
    int ret;
    NetworkInterface *net;
    I2C i2c_lcd(I2C_SDA, I2C_SCL);
    MultiAddrLCD lcd(&i2c_lcd);

    printf("FOTA demo version: %s\n", MBED_CONF_APP_VERSION);

    /* minimal init sequence */
    printf("init platform\n");
    ret = platform_init();
    if (0 != ret) {
        return ret;
    }
    printf("init platform: OK\n");

    /* let the world know we're alive */
    led_set_color(IND_POWER, IND_COLOR_ON);

    lcd.setBacklight(TextLCD_I2C::LightOn);
    lcd.setCursor(TextLCD_I2C::CurOff_BlkOff);
    lcd.printf("Version: %s\n", MBED_CONF_APP_VERSION);

    /* bring up the network */
    printf("init network\n");
    led_set_color(IND_WIFI, IND_COLOR_IN_PROGRESS, true);
    net = init_network();
    if (NULL == net) {
        printf("failed to init network\n");
        lcd.printf("Fail: %s\n", MBED_CONF_APP_WIFI_SSID);
        led_set_color(IND_WIFI, IND_COLOR_FAILED);
        return -ENODEV;
    }
    printf("init network: OK\n");
    lcd.printf("Wifi Connected\n");
    led_set_color(IND_WIFI, IND_COLOR_SUCCESS);

    /* initialize the factory configuration client */
    printf("init factory configuration client\n");
    ret = init_fcc();
    if (0 != ret) {
        printf("failed to init factory configuration client: %d\n", ret);
        return ret;
    }
    printf("init factory configuration client: OK\n");

    /* start the mbed client. does not return */
    printf("starting mbed client\n");
    ret = run_mbed_client(net);
    if (0 != ret) {
        printf("failed to run mbed client: %d\n", ret);
        return ret;
    }

    platform_shutdown();
    printf("exiting main\n");
    return 0;
}

