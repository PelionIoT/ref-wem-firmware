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

#include "lcdprogress.h"
#include "ledman.h"
#include "multiaddrlcd.h"

#include <string>
#include <vector>

#define DISPLAY_UPDATE_PERIOD_MS 180
#define DISPLAY_PAGING_UPDATE_PERIOD_MS 1980 // this should be a multiple of the LED update period

class DisplayMan;

void thread_display_update(DisplayMan *display);

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
    void set_progress(const std::string &message, uint32_t progress,
                      uint32_t total);
    void set_network_status(const std::string &status);
    void set_network_in_progress();
    void set_network_fail();
    void set_network_success();
    void init_network(const char *type);
    /*returns sesor id*/
    uint8_t register_sensor(const std::string &name, enum INDICATOR_TYPES indicator = IND_NO_TYPES);
    void set_sensor_status(uint8_t sensor_id, const std::string status);
    void set_sensor_status(const std::string name, const std::string status);
    void set_sensor_name(uint8_t sensor_id, const std::string name);
    void cycle_status();
    void refresh();
    void self_test();

private:
    struct SensorDisplay {
        std::string name;
        std::string status;
        INDICATOR_TYPES indicator;
    };

    enum ViewMode {
        DISPLAY_VIEW_SENSOR,
        DISPLAY_VIEW_DOWNLOAD,
        DISPLAY_VIEW_INSTALL,
        DISPLAY_VIEW_SELF_TEST
    };

    I2C _i2c;
    MultiAddrLCD _lcd;
    LCDProgress _lcd_prog;

    std::vector<SensorDisplay> _sensors;
    uint8_t _network_sensor_id;
    uint8_t _active_sensor;
    enum ViewMode _view_mode;
    std::string _version_string;
    std::string _network_status;
    bool _cloud_registered;

    uint64_t _cycle_count;

    struct SensorDisplay *find_sensor(const std::string &name);
    void set_sensor_status(struct SensorDisplay *s, const std::string);
};

#endif
