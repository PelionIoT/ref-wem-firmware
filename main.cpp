/**
    Firmware Over The Air (FOTA) demo

    This application demonstrates how to perform fota using mbed cloud 1.2.

    By the ARM Reference Design (Red) Team
*/
#include "mbed.h"
#include <ESP8266Interface.h>
#include "ws2801.h"

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


int COLOR_PASS[] = {
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE
};
int COLOR_FAIL[] = {
        RED, RED, RED, RED, RED, RED, RED, RED,
        RED, RED, RED, RED, RED, RED, RED, RED,
        RED, RED, RED, RED, RED, RED, RED, RED,
        RED, RED, RED, RED, RED, RED, RED, RED
};

// main() runs in its own thread in the OS
int main()
{
    int ret;
    ESP8266Interface wifi(MBED_CONF_APP_WIFI_TX,
                          MBED_CONF_APP_WIFI_RX,
                          MBED_CONF_APP_WIFI_DEBUG);
    ws2801 led_strip(D3, D2, 32);
    led_strip.level(100);

    /* console baudrate */
    set_baud(115200);

    printf("hello world\r\n");

    /* bring up wifi */
    printf("[WIFI] connecting to: %s\n", MBED_CONF_APP_WIFI_SSID);
    ret =  wifi.connect(MBED_CONF_APP_WIFI_SSID,
                        MBED_CONF_APP_WIFI_PASSWORD,
                        wifi_security_str2sec(MBED_CONF_APP_WIFI_SECURITY));
    if (0 != ret) {
        printf("[WIFI] Failed to connect to: %s (%d)\n",
               MBED_CONF_APP_WIFI_SSID, ret);
        led_strip.post(COLOR_FAIL);
        return ret;
    }

    led_strip.post(COLOR_PASS);

    printf("[WIFI] connected to: %s, ip=%s, netmask=%s, gateway=%s\r\n",
           MBED_CONF_APP_WIFI_SSID,
           wifi.get_ip_address(),
           wifi.get_netmask(),
           wifi.get_gateway());

    printf("exiting main\r\n");
    return 0;
}

