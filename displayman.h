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
#include "multiaddrlcd.h"
#include <string>
#include <vector>

class DisplayMan;

void thread_display_update(DisplayMan *display);

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
    void init_network(const char *type);
    /*returns sesor id*/
    uint8_t register_sensor(const char *name);
    void set_sensor_status(uint8_t sensor_id, const char *status);
    void set_sensor_name(uint8_t sensor_id, const char *name);
    void cycle_status();
    MultiAddrLCD& get_lcd();

private:
    I2C                        _i2c;
    MultiAddrLCD               _lcd;
    std::vector<SensorDisplay> _sensors;
    uint8_t                    _next_sensor_id;
    uint8_t                    _network_sensor_id;
    uint8_t                    _active_sensor;
    std::string                _version_string;
};

#endif
