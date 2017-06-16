/**
    Firmware Over The Air (FOTA) demo

    This application demonstrates how to perform fota using mbed cloud 1.2.

    By the ARM Reference Design (Red) Team
*/
#include "mbed.h"
#include "ChainableLED.h"

static void set_baud(int baudrate)
{
    Serial s(USBTX, USBRX);
    s.baud(baudrate);
}

// main() runs in its own thread in the OS
int main()
{
    ChainableLED power_led(D5, D6, 1);

    /* console baudrate */
    set_baud(115200);

    printf("hello world\r\n");
    power_led.setColorRGB(0, 255, 255, 255);
}

