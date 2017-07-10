// ****************************************************************************
//  Firmware Over The Air (FOTA) demo
//
//  This application demonstrates how to perform fota using mbed cloud 1.2.
//
//  By the ARM Reference Design (Red) Team
// ****************************************************************************
#include "ledman.h"

#include <ws2801.h>

static ws2801 led_strip(D3, D2, IND_NO_TYPES);

static int LED_STATUS[IND_NO_TYPES] = {
    IND_COLOR_OFF,
    IND_COLOR_OFF,
    IND_COLOR_OFF,
    IND_COLOR_OFF,
    IND_COLOR_OFF,
    IND_COLOR_OFF,
    IND_COLOR_OFF
};
/* really poor design, but we store the blinkind indicator in the upper 8-bits
 * and the old color in the lower 24-bits. Once blinking is disabled then we
 * just update the LED_STATUS field with the original color.
 */
static int LED_BLINK[IND_NO_TYPES] = {
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0
};

// ****************************************************************************
// Functions
// ****************************************************************************
bool led_blink_is_enabled(int led_name)
{
    return LED_BLINK[led_name] & 0x01000000;
}

void led_blink_enable(int led_name)
{
    LED_BLINK[led_name] |= 0x01000000;
}

void led_blink_disable(int led_name)
{
    LED_BLINK[led_name] &= 0x00FFFFFF;
}

void led_set_color(enum INDICATOR_TYPES led_name, int led_color, bool blink)
{
    /* colors are only 24-bits */
    int color = led_color & 0x00FFFFFF;

    if (led_name >= IND_NO_TYPES) {
        return;
    }

    LED_STATUS[led_name] = color;

    /* save the blink state and only update the color */
    LED_BLINK[led_name] &= 0xFF000000;
    LED_BLINK[led_name] |= color;
    blink ? led_blink_enable(led_name) : led_blink_disable(led_name);
}

void led_post(void)
{
    int idx;

    for (idx = 0; idx < IND_NO_TYPES; ++idx) {
        if (led_blink_is_enabled(idx)) {
            LED_STATUS[idx] = LED_STATUS[idx] == IND_COLOR_OFF ?
                LED_BLINK[idx] & 0x00FFFFFF : IND_COLOR_OFF;
        } else {
            LED_STATUS[idx] = LED_BLINK[idx] & 0x00FFFFFF;
        }
    }
    led_strip.post(LED_STATUS);
}

void led_setup(void)
{
    led_strip.clear();
    led_strip.level(100);
}
