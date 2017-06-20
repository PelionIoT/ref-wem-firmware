/**
    Firmware Over The Air (FOTA) demo

    This application demonstrates how to perform fota using mbed cloud 1.2.

    By the ARM Reference Design (Red) Team
*/
#include "mbed.h"
#include <ESP8266Interface.h>
#include "ChainableLED.h"

static void set_baud(int baudrate)
{
    Serial s(USBTX, USBRX);
    s.baud(baudrate);
}

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

// main() runs in its own thread in the OS
int main()
{
    int ret;
    ChainableLED power_led(D5, D6, 1);
    ESP8266Interface wifi(MBED_CONF_APP_WIFI_TX,
                          MBED_CONF_APP_WIFI_RX,
                          MBED_CONF_APP_WIFI_DEBUG);

    /* console baudrate */
    set_baud(115200);

    printf("hello world\r\n");
    power_led.setColorRGB(0, 255, 255, 255);

    /* bring up wifi */
    printf("[WIFI] connecting to: %s\n", MBED_CONF_APP_WIFI_SSID);
    ret =  wifi.connect(MBED_CONF_APP_WIFI_SSID,
                        MBED_CONF_APP_WIFI_PASSWORD,
                        wifi_security_str2sec(MBED_CONF_APP_WIFI_SECURITY));
    if (0 != ret) {
        printf("[WIFI] Failed to connect to: %s (%d)\n",
               MBED_CONF_APP_WIFI_SSID, ret);
        return ret;
    }
    printf("[WIFI] connected to: %s, ip=%s, netmask=%s, gateway=%s\r\n",
           MBED_CONF_APP_WIFI_SSID,
           wifi.get_ip_address(),
           wifi.get_netmask(),
           wifi.get_gateway());

    printf("exiting main\r\n");
    return 0;
}

