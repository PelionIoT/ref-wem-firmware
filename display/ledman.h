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
    IND_POWER = 0,
    IND_WIFI,
    IND_CLOUD,
    IND_FWUP,
    IND_LIGHT,
    IND_TEMP,
    IND_HUMIDITY = IND_TEMP,
    IND_SOUND,
    IND_NO_TYPES
};

// LEDs are BGR instead of RGB
#define COMMI_RED 0x0000FF
#define CANADIAN_BLUE 0xFF0000
#define SLIMER_GREEN DARKGREEN
#define SNOW_YELLOW 0x00FFFF
#define MIDNIGHT_GREEN 0x003200
#define MIDNIGHT_BLUE 0x0F0000
#define SUMMER_BLUE 0xFEED00
enum INDICATOR_COLORS {
    IND_COLOR_OFF = BLACK,
    IND_COLOR_IN_PROGRESS = SNOW_YELLOW,
    IND_COLOR_FAILED = COMMI_RED,
    IND_COLOR_ON = SLIMER_GREEN,
    IND_COLOR_SUCCESS = CANADIAN_BLUE,
    IND_COLOR_SUCCESS_DIM = MIDNIGHT_BLUE,
    IND_COLOR_SENSOR_DATA = SUMMER_BLUE
};

enum INDICATOR_FLAGS {
    IND_FLAG_NONE = 0,
    IND_FLAG_BLINK = 1,   // blink the LED indefinitely
    IND_FLAG_ONCE = 2,    // show the color once and then revert to the previous color
    IND_FLAG_LATER = 4,   // show the color next cycle
};

void led_flags_set(int led_name, int flags);  // overwrite all flags
int led_flags_get(int led_name);              // return all flags
bool led_flag_is_set(int led_name, int flag); // check whether a particular flag is set
void led_clear_flag(int led_name, int flag);  // clear one flag
void led_set_color(enum INDICATOR_TYPES led_name, int led_color, int flags = IND_FLAG_NONE);
void led_post(void);                          // update hardware LEDs
void led_setup(void);
