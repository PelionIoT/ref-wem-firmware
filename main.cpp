// ****************************************************************************
//  Firmware Over The Air (FOTA) demo
//
//  This application demonstrates how to perform fota using mbed cloud 1.2.
//
//  By the ARM Reference Design (Red) Team
// ****************************************************************************
#include "m2mclient.h"

#include "GL5528.h"
#include "displayman.h"
#include "DHT.h"
#include "lcdprogress.h"

#include <errno.h>
#include <factory_configurator_client.h>
#include <mbed-trace-helper.h>
#include <mbed-trace/mbed_trace.h>
#include <SDBlockDevice.h>

#if MBED_CONF_APP_WIFI
    #include <ESP8266Interface.h>
#else
    #include <EthernetInterface.h>
#endif

#define TRACE_GROUP  "main"

// ****************************************************************************
// DEFINEs and type definitions
// ****************************************************************************
enum FOTA_THREADS {
    FOTA_THREAD_DISPLAY = 0,
    FOTA_THREAD_SENSOR_LIGHT,
    FOTA_THREAD_THERMO,
    FOTA_THREAD_DHT,
    FOTA_THREAD_COUNT
};

// ****************************************************************************
// Globals
// ****************************************************************************
/* declared in pal_plat_fileSystem.cpp, which is included because COMMON_PAL
 * is defined in mbed_app.json */
extern SDBlockDevice sd;
DisplayMan display;
M2MClient *gmbed_client;
NetworkInterface *gnet;
LCDProgress lcd_prog(display.get_lcd());

Thread tman[FOTA_THREAD_COUNT];

// ****************************************************************************
// Threads
// ****************************************************************************
static void thread_light_sensor(M2MClient *mbed_client)
{
    M2MObject *light_obj;
    M2MObjectInstance *light_inst;
    M2MResource *light_res;

    using namespace fota::sensor;
    uint8_t res_buffer[33] = {0};
    int size = 0;
    light::LightSensor<light::BOARD_GROVE_GL5528> light(A0);
    int light_id = display.register_sensor("Light");

    /* register the m2m object */
    light_obj = M2MInterfaceFactory::create_object("5002");
    light_inst = light_obj->create_object_instance();

    light_res = light_inst->create_dynamic_resource("1", "light_resource",
            M2MResourceInstance::FLOAT, true /* observable */);
    light_res->set_operation(M2MBase::GET_ALLOWED);
    light_res->set_value((uint8_t *)"0", 1);

    mbed_client->add_resource(light_obj);

    while (true) {
        light.update();
        float flux = light.getFlux();

        size = sprintf((char *)res_buffer,"%2.2f", flux);

        printf("reading: %2.2f\r\n", flux);
        display.set_sensor_status(light_id, (char *)res_buffer);
        light_res->set_value(res_buffer, size);
        Thread::wait(500);
    }
}

static void thread_thermo(M2MClient *mbed_client)
{
    M2MObject *thermo_obj;
    M2MObjectInstance *thermo_inst;
    M2MResource *thermo_res;

    uint8_t res_buffer[33] = {0};
    int size = 0;

    AnalogIn thermistor(A1);

    /* Assuming Grove temperature sensor 1.2 is attached to A1 */
    float beta = 4275.0;
    float val, resistance, temperature;

    /* register the m2m object */
    thermo_obj = M2MInterfaceFactory::create_object("5005");
    thermo_inst = thermo_obj->create_object_instance();

    thermo_res = thermo_inst->create_dynamic_resource("1", "thermo_resource",
            M2MResourceInstance::FLOAT, true /* observable */);
    thermo_res->set_operation(M2MBase::GET_ALLOWED);
    thermo_res->set_value((uint8_t *)"0", 1);

    mbed_client->add_resource(thermo_obj);

    while (true) {
        /* Assume 5V VCC */
        val = thermistor.read() / 5.0 * 3.3;

        /* resistance is in unit of 10k ohms */
        resistance = 1.0 / val - 1.0;

        /* beta equation */
        temperature = 1 / (log(resistance) / beta + 1.0 / 298.15) - 273.15;

        tr_debug("thermistor: val = %f, temp = %f\n", val, temperature);
        size = sprintf((char *)res_buffer, "%.0f", temperature);
        thermo_res->set_value(res_buffer, size);
        Thread::wait(1000);
    }
}

static void thread_dht(M2MClient *mbed_client)
{
    M2MObject *dht_obj;
    M2MObjectInstance *dht_inst;
    M2MResource *dht_res;

    uint8_t res_buffer[33] = {0};
    int size = 0;

    AnalogIn thermistor(A1);
    DHT dht(D4, AM2302);
    eError readError;
    float temperature, humidity;

    /* register the m2m object */
    dht_obj = M2MInterfaceFactory::create_object("5006");
    dht_inst = dht_obj->create_object_instance();

    dht_res = dht_inst->create_dynamic_resource("1", "humidity_resource",
            M2MResourceInstance::FLOAT, true /* observable */);
    dht_res->set_operation(M2MBase::GET_ALLOWED);
    dht_res->set_value((uint8_t *)"0", 1);

    mbed_client->add_resource(dht_obj);


    while (true) {
        readError = dht.readData();
        if (readError == ERROR_NONE) {
            temperature = dht.ReadTemperature(CELCIUS);
            humidity = dht.ReadHumidity();
            tr_debug("DHT: temp = %fC, humi = %f%%\n", temperature, humidity);
            size = sprintf((char *)res_buffer, "%.0f", humidity);
            dht_res->set_value(res_buffer, size);
        } else {
            tr_error("DHT: readData() failed with %d\n", readError);
        }
        Thread::wait(1000);
    }
}

static void start_sensors(M2MClient *mbed_client)
{
    printf("starting all sensors\n");
    tman[FOTA_THREAD_SENSOR_LIGHT].start(callback(thread_light_sensor, mbed_client));
    tman[FOTA_THREAD_THERMO].start(callback(thread_thermo, mbed_client));
    tman[FOTA_THREAD_DHT].start(callback(thread_dht, mbed_client));
}

static void stop_sensors()
{
    printf("stopping all sensors\n");
    tman[FOTA_THREAD_SENSOR_LIGHT].terminate();
    tman[FOTA_THREAD_THERMO].terminate();
    tman[FOTA_THREAD_DHT].terminate();
}

// ****************************************************************************
// Cloud
// ****************************************************************************
void mbed_client_on_update_authorize(int32_t request)
{
    M2MClient *mbed_client = gmbed_client;
    int ret;
    MultiAddrLCD& lcd = display.get_lcd();

    switch (request) {
        /* Cloud Client wishes to download new firmware. This can have a
         * negative impact on the performance of the rest of the system.
         *
         * The user application is supposed to pause performance sensitive tasks
         * before authorizing the download.
         *
         * Note: the authorization call can be postponed and called later.
         * This doesn't affect the performance of the Cloud Client.
         * */
        case MbedCloudClient::UpdateRequestDownload:
            printf("Firmware download requested\r\n");
            printf("Authorization granted\r\n");
            stop_sensors();
            tman[FOTA_THREAD_DISPLAY].terminate();
            mbed_client->update_authorize(request);
            led_set_color(IND_FWUP, IND_COLOR_IN_PROGRESS, true);
            break;

        /* Cloud Client wishes to reboot and apply the new firmware.
         *
         * The user application is supposed to save all current work before
         * rebooting.
         *
         * Note: the authorization call can be postponed and called later.
         * This doesn't affect the performance of the Cloud Client.
         * */
        case MbedCloudClient::UpdateRequestInstall:
            printf("Firmware install requested\r\n");
            printf("Disconnecting network...\n");
            ret = gnet->disconnect();
            printf("Network disconnect returned with %d\n", ret);
            lcd.printline(0, "Installing...    ");
            lcd.printline(1, "");
            printf("Authorization granted\r\n");
            mbed_client->update_authorize(request);
            led_set_color(IND_FWUP, IND_COLOR_IN_PROGRESS, true);
            break;

        default:
            printf("Error - unknown request\r\n");
            led_set_color(IND_FWUP, IND_COLOR_FAILED);
            break;
    }
}

void mbed_client_on_update_progress(uint32_t progress, uint32_t total)
{
    uint32_t percent = progress * 100 / total;
    static uint32_t last_percent = 0;
    const char dl_message[] = "Downloading...";
    const char done_message[] = "Saving...";

    /* Drive the LCD in the main thread to prevent network corruption */
    lcd_prog.set_progress(dl_message, progress, total);

    if (last_percent < percent) {
        printf("Downloading: %lu\n", percent);
    }

    if (progress == total) {
        printf("\r\nDownload completed\r\n");
        lcd_prog.set_progress(done_message, 0, 100);
        led_set_color(IND_FWUP, IND_COLOR_SUCCESS);
    }

    last_percent = percent;
}

static void mbed_client_on_registered(void *context)
{
    printf("mbed client registered\n");
    display.set_cloud_registered();
}

static void mbed_client_on_unregistered(void *context)
{
    printf("mbed client unregistered\n");
    display.set_cloud_unregistered();
}

static void mbed_client_on_error(void *context)
{
    printf("mbed client ERROR\n");
    display.set_cloud_error();
}

static int run_mbed_client(NetworkInterface *iface,
                           M2MClient *mbed_client)
{
    mbed_client->on_registered(NULL, mbed_client_on_registered);
    mbed_client->on_unregistered(NULL, mbed_client_on_unregistered);
    mbed_client->on_error(NULL, mbed_client_on_error);
    mbed_client->on_update_authorize(mbed_client_on_update_authorize);
    mbed_client->on_update_progress(mbed_client_on_update_progress);

    printf("mbed client: connecting\n");
    display.set_cloud_in_progress();
    mbed_client->call_register(iface);

    printf("mbed client: entering run loop\n");
    while (mbed_client->is_register_called()) {
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

// ****************************************************************************
// Network
// ****************************************************************************
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

// ****************************************************************************
// Generic Helpers
// ****************************************************************************
static int platform_init(M2MClient *mbed_client)
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

    /* setup the display */
    display.init();
    tman[FOTA_THREAD_DISPLAY].start(callback(thread_display_update, &display));

    return 0;
}

static void platform_shutdown()
{
    rtos::Thread::State state;

    state = tman[FOTA_THREAD_DISPLAY].get_state();
    if (rtos::Thread::Running == state) {
        tman[FOTA_THREAD_DISPLAY].join();
    }

    state = tman[FOTA_THREAD_SENSOR_LIGHT].get_state();
    if (rtos::Thread::Running == state) {
        tman[FOTA_THREAD_SENSOR_LIGHT].join();
    }

    state = tman[FOTA_THREAD_THERMO].get_state();
    if (rtos::Thread::Running == state) {
        tman[FOTA_THREAD_THERMO].join();
    }

    state = tman[FOTA_THREAD_DHT].get_state();
    if (rtos::Thread::Running == state) {
        tman[FOTA_THREAD_DHT].join();
    }
}

#if MBED_CONF_APP_SELF_TEST
static void self_test()
{
    I2C i2c(I2C_SDA, I2C_SCL);
    MultiAddrLCD lcd(&i2c);
    lcd.setBacklight(TextLCD_I2C::LightOn);
    lcd.setCursor(TextLCD_I2C::CurOff_BlkOff);
    LCDProgress prog(lcd);
    for (uint32_t i = 0; i <= 150; i++) {
        prog.set_progress("Starting", i, 150);
        Thread::wait(10);
    }
}
#endif

// ****************************************************************************
// Main
// main() runs in its own thread in the OS
// ****************************************************************************
int main()
{
    int ret;
    M2MClient *mbed_client;

#if MBED_CONF_APP_SELF_TEST
    self_test();
#endif

    printf("FOTA demo version: %s\n", MBED_CONF_APP_VERSION);
    gmbed_client = new M2MClient();
    mbed_client = gmbed_client;

    /* minimal init sequence */
    printf("init platform\n");
    ret = platform_init(mbed_client);
    if (0 != ret) {
        return ret;
    }
    printf("init platform: OK\n");

    /* let the world know we're alive */
    display.set_power_on();
    display.set_version_string(MBED_CONF_APP_VERSION);

    /* bring up the network */
    printf("init network\n");
    display.set_network_in_progress();
    gnet = init_network();
    if (NULL == gnet) {
        printf("failed to init network\n");
        display.set_network_fail();
        return -ENODEV;
    }
    printf("init network: OK\n");
    display.set_network_success();

    /* initialize the factory configuration client */
    printf("init factory configuration client\n");
    ret = init_fcc();
    if (0 != ret) {
        printf("failed to init factory configuration client: %d\n", ret);
        return ret;
    }
    printf("init factory configuration client: OK\n");

    /* start sampling */
    start_sensors(mbed_client);

    /* start the mbed client. does not return */
    printf("starting mbed client\n");
    ret = run_mbed_client(gnet, mbed_client);
    if (0 != ret) {
        printf("failed to run mbed client: %d\n", ret);
        return ret;
    }

    stop_sensors();
    platform_shutdown();
    printf("exiting main\n");
    return 0;
}

