// ****************************************************************************
//  Firmware Over The Air (FOTA) demo
//
//  This application demonstrates how to perform fota using mbed cloud 1.2.
//
//  By the ARM Reference Design (Red) Team
// ****************************************************************************
#include "HTML_color.h"

// ****************************************************************************
// DEFINEs and type definitions
// ****************************************************************************
enum INDICATOR_TYPES {
    IND_LCD = 0,
    IND_CLOUD,
    IND_POWER,
    IND_WIFI,
    IND_FWUP,
    IND_NO_TYPES
};

#define COMMI_RED RED
#define CANADIAN_BLUE BLUE
enum INDICATOR_COLORS {
    IND_COLOR_OFF = BLACK,
    IND_COLOR_IN_PROGRESS = YELLOW,
    IND_COLOR_FAILED = COMMI_RED,
    IND_COLOR_ON = DARKGREEN,
    IND_COLOR_SUCCESS = CANADIAN_BLUE
};

bool led_blink_is_enabled(int led_name);
void led_blink_enable(int led_name);
void led_blink_disable(int led_name);
void led_set_color(enum INDICATOR_TYPES led_name, int led_color, bool blink = false);
void led_post(void);
void led_setup(void);
