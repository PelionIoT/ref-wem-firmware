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
#include "lcdprogress.h"
#include "multiaddrlcd.h"

#include <string>
#include <vector>

class DisplayMan;

void thread_display_update(DisplayMan *display);

struct SensorDisplay {
    std::string name;
    std::string status;
};

enum ViewMode {
    DISPLAY_VIEW_SENSOR,
    DISPLAY_VIEW_DOWNLOAD,
    DISPLAY_VIEW_INSTALL,
    DISPLAY_VIEW_SELF_TEST
};

class DisplayMan
{
public:
    DisplayMan();
    int init(const std::string &version);
    void set_cloud_registered();
    void set_cloud_unregistered();
    void set_cloud_in_progress();
    void set_cloud_error();
    void set_downloading();
    void set_download_complete();
    void set_installing();
    void set_progress(const std::string &message, uint32_t progress, uint32_t total);
    void set_network_in_progress();
    void set_network_fail();
    void set_network_success();
    void init_network(const char *type);
    /*returns sesor id*/
    uint8_t register_sensor(const std::string &name);
    void set_sensor_status(uint8_t sensor_id, const std::string status);
    void set_sensor_name(uint8_t sensor_id, const std::string name);
    void cycle_status();
    void refresh();
    void self_test();

private:
    I2C                         _i2c;
    MultiAddrLCD                _lcd;
    LCDProgress                 _lcd_prog;

    std::vector<SensorDisplay>  _sensors;
    uint8_t                    _network_sensor_id;
    uint8_t                    _active_sensor;
    enum ViewMode               _view_mode;
    std::string                 _version_string;

    uint64_t _cycle_count;
};

#endif
