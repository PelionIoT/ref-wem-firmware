// ****************************************************************************
//  Firmware Over The Air (FOTA) demo
//
//  This application demonstrates how to perform fota using mbed cloud 1.2.
//
//  By the ARM Reference Design (Red) Team
// ****************************************************************************
#include "m2mclient.h"

#include <errno.h>
#include <factory_configurator_client.h>
#include <SDBlockDevice.h>
#include <TextLCD.h>
#include <ws2801.h>

#if MBED_CONF_APP_WIFI
    #include <ESP8266Interface.h>
#else
    #include <EthernetInterface.h>
#endif

// ****************************************************************************
// DEFINEs and type definitions
// ****************************************************************************

// ****************************************************************************
// Globals
// ****************************************************************************
/* declared in pal_plat_fileSystem.cpp, which is included because COMMON_PAL
 * is defined in mbed_app.json */
extern SDBlockDevice sd;

int COLOR_POWER[] = {
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE
};

int COLOR_CONNECTED[] = {
        RED, RED, RED, RED, RED, RED, RED, RED,
        RED, RED, RED, RED, RED, RED, RED, RED,
        RED, RED, RED, RED, RED, RED, RED, RED,
        RED, RED, RED, RED, RED, RED, RED, RED
};

// ****************************************************************************
// Functions
// ****************************************************************************
static int init_platform()
{
    int ret;

    /* init the sd card */
    ret = sd.init();
    if (ret != BD_ERROR_OK) {
        printf("sd init failed: %d\n", ret);
        return ret;
    }
    printf("sd init OK\n");

    return 0;
}

static int run_mbed_client(NetworkInterface *iface)
{
    M2MClient mbed_client;

    mbed_client.create_resources();

    printf("mbed client: connecting\n");
    mbed_client.call_register(iface);

    printf("mbed client: waiting for register called\n");
    while (mbed_client.is_register_called()) {
        Thread::wait(1000);
    }
    printf("mbed client: waiting for register called: DONE\n");

    return 0;
}

static int init_fcc(void)
{
    fcc_status_e ret;

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
    ws2801 led_strip(D3, D2, 32);
    I2C i2c_lcd(I2C_SDA, I2C_SCL);
    TextLCD_I2C lcd(&i2c_lcd, 0x7e, TextLCD::LCD16x2, TextLCD::HD44780);

    printf("FOTA demo version: %s\n", MBED_CONF_APP_VERSION);

    /* minimal init sequence */
    printf("init platform\n");
    ret = init_platform();
    if (0 != ret) {
        return ret;
    }
    printf("init platform: OK\n");

    /* let the world know we're alive */
    led_strip.post(COLOR_POWER);
    led_strip.level(100);

    /* bring up the network */
    printf("init network\n");
    net = init_network();
    if (NULL == net) {
        printf("failed to init network\n");
        lcd.printf("Wifi fail");
        return -ENODEV;
    }
    printf("init network: OK\n");
    lcd.printf("Wifi Connected");
    led_strip.post(COLOR_CONNECTED);

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

    printf("exiting main\n");
    return 0;
}

