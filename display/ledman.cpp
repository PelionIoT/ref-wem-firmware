// ****************************************************************************
//  Firmware Over The Air (FOTA) demo
//
//  This application demonstrates how to perform fota using mbed cloud 1.2.
//
//  By the ARM Reference Design (Red) Team
// ****************************************************************************
#include "ledman.h"

#include <ws2801.h>

#if TARGET_UBLOX_EVK_ODIN_W2
static ws2801 led_strip(D7, D6, IND_NO_TYPES);
#else
static ws2801 led_strip(D3, D2, IND_NO_TYPES);
#endif

// colors being displayed
static int LED_HARDWARE[IND_NO_TYPES] = {
    IND_COLOR_OFF, IND_COLOR_OFF, IND_COLOR_OFF, IND_COLOR_OFF,
    IND_COLOR_OFF, IND_COLOR_OFF, IND_COLOR_OFF};
/* really poor design, but we store the indicator flags in the upper 8-bits
 * and the color in the lower 24-bits. Once blinking is disabled then we
 * just update the LED_HARDWARE field with the original color.
 */
static int LED_COLOR[IND_NO_TYPES] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
// temporary color
static int LED_COLOR_TEMP[IND_NO_TYPES] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

// ****************************************************************************
// Functions
// ****************************************************************************
void led_flags_set(int led_name, int flags)
{
    LED_COLOR[led_name] &= 0x00FFFFFF;
    LED_COLOR[led_name] |= flags << 24;
    LED_COLOR_TEMP[led_name] &= 0x00FFFFFF;
    LED_COLOR_TEMP[led_name] |= flags << 24;
}

int led_flags_get(int led_name)
{
    return LED_COLOR[led_name] >> 24;
}

bool led_flag_is_set(int led_name, int flag)
{
    return led_flags_get(led_name) & flag;
}

void led_clear_flag(int led_name, int flag)
{
    LED_COLOR[led_name] &= ~(flag << 24);
    LED_COLOR_TEMP[led_name] &= ~(flag << 24);
}

void led_set_color(enum INDICATOR_TYPES led_name, int led_color, int flags)
{
    /* colors are only 24-bits */
    int color = led_color & 0x00FFFFFF;

    if (led_name >= IND_NO_TYPES) {
        return;
    }

    if ((flags & IND_FLAG_ONCE) || (flags & IND_FLAG_LATER)) {
        LED_COLOR_TEMP[led_name] = color;
    } else {
        LED_COLOR[led_name] = color;
    }
    led_flags_set(led_name, flags);
}

void led_post(void)
{
    int idx;

    for (idx = 0; idx < IND_NO_TYPES; ++idx) {
        int color = LED_COLOR[idx];
        if (led_flag_is_set(idx, IND_FLAG_BLINK) && LED_HARDWARE[idx] != IND_COLOR_OFF) {
            color = IND_COLOR_OFF;
        }
        if (led_flag_is_set(idx, IND_FLAG_LATER)) {
            if (!led_flag_is_set(idx, IND_FLAG_ONCE)) {
                LED_COLOR[idx] = LED_COLOR_TEMP[idx];
            }
            led_clear_flag(idx, IND_FLAG_LATER);
        } else if (led_flag_is_set(idx, IND_FLAG_ONCE)) {
            color = LED_COLOR_TEMP[idx];
            led_clear_flag(idx, IND_FLAG_ONCE);
        }
        LED_HARDWARE[idx] = color & 0x00FFFFFF;
    }
    led_strip.post(LED_HARDWARE);
}

void led_setup(void)
{
    for (int i = 0; i < IND_NO_TYPES; i++) {
        LED_HARDWARE[i] = IND_COLOR_OFF;
        LED_COLOR[i] = 0x0;
        LED_COLOR_TEMP[i] = 0x0;
    }
    led_strip.clear();
    led_strip.level(100);
}
