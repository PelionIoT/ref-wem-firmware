// ****************************************************************************
//  Firmware Over The Air (FOTA) demo
//
//  This application demonstrates how to perform fota using mbed cloud 1.2.
//
//  By the ARM Reference Design (Red) Team
// ****************************************************************************

// Abstraction of hardware display

#ifndef __DISPLAYMAN_H__
#define __DISPLAYMAN_H__
#include "ledman.h"
#include <TextLCD.h>
#include <string>
#include <vector>

class DisplayMan;

void thread_display_update(DisplayMan *display);


//LCD which can have I2C slave address of eith 0x4e or 0x7e
class MultiAddrLCD
{
public:
    MultiAddrLCD(I2C *i2c);

    /*Only supporting 16x2 LCDs, so string will be truncated at 32
      characters.*/
    int printf(const char *format, ...);
    /*Print on the given line (0 or 1)*/
    int printline(int line, const char *format, ...);
    void setBacklight(TextLCD_Base::LCDBacklight mode);
    void setCursor(TextLCD_Base::LCDCursor mode);

private:
    TextLCD_I2C _lcd1;
    TextLCD_I2C _lcd2;
};

struct SensorDisplay {
    std::string name;
    std::string status;
};


class DisplayMan
{
public:
    DisplayMan();
    int init();
    void set_version_string(const char *version);
    void set_power_on();
    void set_cloud_in_progress();
    void set_cloud_registered();
    void set_cloud_unregistered();
    void set_cloud_error();
    void set_network_in_progress();
    void set_network_fail();
    void set_network_success();
    /*returns sesor id*/
    int register_sensor(const char *name);
    void set_sensor_status(int sensor_id, char *status);
    void cycle_status();

private:
    I2C                        _i2c;
    MultiAddrLCD               _lcd;
    std::vector<SensorDisplay> _sensors;
    int                        _next_sensor_id;
    int                        _network_sensor_id;
    int                        _active_sensor;
};

#endif
