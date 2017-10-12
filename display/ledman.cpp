// ****************************************************************************
//  Firmware Over The Air (FOTA) demo
//
//  This application demonstrates how to perform fota using mbed cloud 1.2.
//
//  By the ARM Reference Design (Red) Team
// ****************************************************************************
#include "ledman.h"

#include <ws2801.h>

/* Supported boards for different LEDControllers
 */
enum TARGET_BOARDS {
    TARGET_BOARD_K64F,
    TARGET_BOARD_UBLOX_EVK,
    TARGET_BOARD_COUNT
};

/* LEDController template logic.
 */
class BaseController
{
public:
    BaseController() { }
    ~BaseController() { }

    /* Set flag for particular LED.
     *
     * @param led_name Which LED to set the flag on.
     * @param flags The flag to set.
     */
    void flags_set(int led_name, int flags) {
        LED_COLOR[led_name] &= 0x00FFFFFF;
        LED_COLOR[led_name] |= flags << 24;
        LED_COLOR_TEMP[led_name] &= 0x00FFFFFF;
        LED_COLOR_TEMP[led_name] |= flags << 24;
    }

    /* Get the current flags set on a LED.
     *
     * @param led_name Which LED to return flags from.
     * @return Returns the flags currently set on led_name.
     */
    int flags_get(int led_name) {
        return LED_COLOR[led_name] >> 24;
    }

    /* Check is a flag is currently set on a LED.
     *
     * @param led_name Which LED to check for the flag.
     * @param flag The particular flag to check for.
     * @return Returns true if the flag is set, false otherwise.
     */
    bool flag_is_set(int led_name, int flag) {
        return flags_get(led_name) & flag;
    }

    /* Clears a particular flag on a LED.
     *
     * @param led_name Which LED to clear the flag on.
     * @param flag The flag to clear.
     */
    void clear_flag(int led_name, int flag) {
        LED_COLOR[led_name] &= ~(flag << 24);
        LED_COLOR_TEMP[led_name] &= ~(flag << 24);
    }

    /* Set a particular color and flag on a LED.
     *
     * @param led_name Which LED to set the flag and change color.
     * @param led_color The color of the LED.
     * @param flags The flags to set on the LED.
     */
    void set_color(enum INDICATOR_TYPES led_name, int led_color, int flags) {
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
        flags_set(led_name, flags);
    }

    void led_post(void) {
        int idx;

        for (idx = 0; idx < IND_NO_TYPES; ++idx) {
            int color = LED_COLOR[idx];
            if (flag_is_set(idx, IND_FLAG_BLINK) && LED_HARDWARE[idx] != IND_COLOR_OFF) {
                color = IND_COLOR_OFF;
            }
            if (flag_is_set(idx, IND_FLAG_LATER)) {
                if (!flag_is_set(idx, IND_FLAG_ONCE)) {
                    LED_COLOR[idx] = LED_COLOR_TEMP[idx];
                }
                clear_flag(idx, IND_FLAG_LATER);
            } else if (flag_is_set(idx, IND_FLAG_ONCE)) {
                color = LED_COLOR_TEMP[idx];
                clear_flag(idx, IND_FLAG_ONCE);
            }
            LED_HARDWARE[idx] = color & 0x00FFFFFF;
        }
        led_update();
    }

    void led_setup(void) {
        for (int i = 0; i < IND_NO_TYPES; i++) {
            LED_HARDWARE[i] = IND_COLOR_OFF;
            LED_COLOR[i] = 0x0;
            LED_COLOR_TEMP[i] = 0x0;
        }
        led_init();
    }

protected:
    /* Virtual function for updating LEDs which derived classes should define. */
    virtual void led_update(void) = 0;
    /* Virtual function for setting up LEDs which derviced classes should define. */
    virtual void led_init(void) = 0;

    int LED_HARDWARE[IND_NO_TYPES]; /* color currently being displayed */
    /* really poor design, but we store the indicator flags in the upper 8-bits
     * and the color in the lower 24-bits. Once blinking is disabled then we
     * just update the LED_HARDWARE field with the original color.
     */
    int LED_COLOR[IND_NO_TYPES];
    int LED_COLOR_TEMP[IND_NO_TYPES]; /* tempoary colors */
};

/** Template class for creating new LEDControllers
 */
template<enum TARGET_BOARDS>
class LEDController
{
};

/* K64F specific LED Controller.
 */
template<>
class LEDController<TARGET_BOARD_K64F> : public BaseController
{
public:
    LEDController() : led_strip(D3, D2, IND_NO_TYPES) { }
    ~LEDController() { }

protected:
    /** Updates the hardware LEDs based on the internal colors and flags set.
     */
    void led_post(void) {
        led_strip.post(LED_HARDWARE);
    }

    /** Setup the internal state of the LED colors and flags.
     */
    void led_setup(void) {
        led_strip.clear();
        led_strip.level(100);
    }

    ws2801 led_strip;
};

/* UBLOX_EVK_ODIN_W2 specific LED Controller
 */
template<>
class LEDController<TARGET_BOARD_UBLOX_EVK> : public BaseController
{
public:
    LEDController() : led_strip(D7, D6, IND_NO_TYPES * 2) { }
    ~LEDController() { }

protected:
    /** Updates the hardware LEDs based on the internal colors and flags set.
     */
    void led_update(void) {
        int idx;

        for (idx = 0; idx < IND_NO_TYPES; ++idx) {
            switch (LED_HARDWARE[idx]) {
            case COMMI_RED:
                LED_HARDWARE_ODIN[2 * idx] = COMMI_RED_HI;
                LED_HARDWARE_ODIN[2 * idx + 1] = COMMI_RED_LO;
                break;
            case CANADIAN_BLUE:
                LED_HARDWARE_ODIN[2 * idx] = CANADIAN_BLUE_HI;
                LED_HARDWARE_ODIN[2 * idx + 1] = CANADIAN_BLUE_LO;
                break;
            case SLIMER_GREEN:
                LED_HARDWARE_ODIN[2 * idx] = SLIMER_GREEN_HI;
                LED_HARDWARE_ODIN[2 * idx + 1] = SLIMER_GREEN_LO;
                break;
            case SNOW_YELLOW:
                LED_HARDWARE_ODIN[2 * idx] = SNOW_YELLOW_HI;
                LED_HARDWARE_ODIN[2 * idx + 1] = SNOW_YELLOW_LO;
                break;
            case MIDNIGHT_GREEN:
                LED_HARDWARE_ODIN[2 * idx] = MIDNIGHT_GREEN_HI;
                LED_HARDWARE_ODIN[2 * idx + 1] = MIDNIGHT_GREEN_LO;
                break;
            case MIDNIGHT_BLUE:
                LED_HARDWARE_ODIN[2 * idx] = MIDNIGHT_BLUE_HI;
                LED_HARDWARE_ODIN[2 * idx + 1] = MIDNIGHT_BLUE_LO;
                break;
            case SUMMER_BLUE:
                LED_HARDWARE_ODIN[2 * idx] = SUMMER_BLUE_HI;
                LED_HARDWARE_ODIN[2 * idx + 1] = SUMMER_BLUE_LO;
                break;
            default:
                LED_HARDWARE_ODIN[2 * idx] = 0;
                LED_HARDWARE_ODIN[2 * idx + 1] = 0;
            }
        }
        led_strip.post(LED_HARDWARE_ODIN);
    }

    /** Setup the internal state of the LED colors and flags.
     */
    void led_init(void) {
        led_strip.clear();
        led_strip.level(100);
    }

    /* ODIN is slightly different in the way it needs to set the LED colors.
     * We need a separate array to transmit colors to the hardware.
     */
    int LED_HARDWARE_ODIN[IND_NO_TYPES * 2]; /* color currently being displayed */
    ws2801 led_strip;
};

#if TARGET_UBLOX_EVK_ODIN_W2
LEDController<TARGET_BOARD_UBLOX_EVK> ledctrl;
#else
LEDController<TARGET_BOARD_K64F> ledctrl;
#endif

// ****************************************************************************
// Functions
// ****************************************************************************
void led_flags_set(int led_name, int flags) {
    ledctrl.flags_set(led_name, flags);
}

int led_flags_get(int led_name) {
    return ledctrl.flags_get(led_name);
}

bool led_flag_is_set(int led_name, int flag) {
    return ledctrl.flag_is_set(led_name, flag);
}

void led_clear_flag(int led_name, int flag) {
    ledctrl.clear_flag(led_name, flag);
}

void led_set_color(enum INDICATOR_TYPES led_name, int led_color, int flags) {
    ledctrl.set_color(led_name, led_color, flags);
}

void led_post(void) {
    ledctrl.led_post();
}

void led_setup(void) {
    ledctrl.led_setup();
}
