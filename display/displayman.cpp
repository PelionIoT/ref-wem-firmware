#include "displayman.h"

DisplayMan::DisplayMan() : _i2c(I2C_SDA, I2C_SCL), _lcd(&_i2c), _lcd_prog(_lcd)
{
    _cycle_count = 0;
    _view_mode = DISPLAY_VIEW_SENSOR;
    _network_sensor_id = UINT8_MAX;
}

int DisplayMan::init(const std::string &version)
{
    _version_string = version;
    led_setup();
    led_set_color(IND_POWER, IND_COLOR_ON);
    _lcd.setBacklight(TextLCD_I2C::LightOn);
    _lcd.setCursor(TextLCD_I2C::CurOff_BlkOff);
#if MBED_CONF_APP_SELF_TEST
    self_test();
    led_setup();
#endif
    _view_mode = DISPLAY_VIEW_SENSOR;
    return 0;
}

void DisplayMan::set_downloading()
{
    _view_mode = DISPLAY_VIEW_DOWNLOAD;
    led_set_color(IND_FWUP, IND_COLOR_IN_PROGRESS, true);
}

void DisplayMan::set_download_complete()
{
    led_set_color(IND_FWUP, IND_COLOR_SUCCESS);
}

void DisplayMan::set_progress(const std::string &message, uint32_t progress,
                              uint32_t total)
{
    _lcd_prog.set_progress(message, progress, total);
}

void DisplayMan::set_cloud_registered()
{
    led_set_color(IND_CLOUD, IND_COLOR_SUCCESS);
}

void DisplayMan::set_cloud_unregistered()
{
    led_set_color(IND_CLOUD, IND_COLOR_OFF);
}

void DisplayMan::set_installing()
{
    _view_mode = DISPLAY_VIEW_INSTALL;
    _lcd.printline(0, "Installing...    ");
    _lcd.printline(1, "");
    led_set_color(IND_FWUP, IND_COLOR_SUCCESS, false);
}

void DisplayMan::set_cloud_error()
{
    led_set_color(IND_CLOUD, IND_COLOR_FAILED);
}

void DisplayMan::init_network(const char *type)
{
    if (UINT8_MAX == _network_sensor_id) {
        _network_sensor_id = register_sensor(type);
    } else {
        set_sensor_name(_network_sensor_id, type);
    }
}

void DisplayMan::set_network_in_progress()
{
    led_set_color(IND_WIFI, IND_COLOR_IN_PROGRESS, true);
    set_sensor_status(_network_sensor_id, MBED_CONF_APP_WIFI_SSID);
}

void DisplayMan::set_network_fail()
{
    led_set_color(IND_WIFI, IND_COLOR_FAILED);
    set_sensor_status(_network_sensor_id, MBED_CONF_APP_WIFI_SSID);
}

void DisplayMan::set_network_success()
{
    led_set_color(IND_WIFI, IND_COLOR_SUCCESS);
    set_sensor_status(_network_sensor_id, MBED_CONF_APP_WIFI_SSID);
}

void DisplayMan::set_cloud_in_progress()
{
    led_set_color(IND_CLOUD, IND_COLOR_IN_PROGRESS, true);
}

uint8_t DisplayMan::register_sensor(const std::string &name)
{
    struct SensorDisplay s;

    s.name = name;
    s.status = "";

    _sensors.push_back(s);

    return _sensors.size() - 1;
}

void DisplayMan::set_sensor_status(uint8_t sensor_id, const std::string status)
{
    if (sensor_id < _sensors.size()) {
        _sensors[sensor_id].status = status;
    }
}

void DisplayMan::set_sensor_name(uint8_t sensor_id, const std::string name)
{
    if (sensor_id < _sensors.size()) {
        _sensors[sensor_id].name = name;
    }
}

void DisplayMan::cycle_status()
{
    char line[17];

    /* top line */
    _lcd.printline(0, "Version: %s", _version_string.c_str());

    /* bottom line */
    if (_sensors.size() > 0) {
        snprintf(line, 16, "%s: %s", _sensors[_active_sensor].name.c_str(),
                 _sensors[_active_sensor].status.c_str());
        _lcd.printline(1, line);
        _active_sensor = (_active_sensor + 1) % _sensors.size();
    }
}

void DisplayMan::refresh()
{
    led_post();

    if (_view_mode == DISPLAY_VIEW_SENSOR) {
        if ((_cycle_count & 0x7) == 0) {
            cycle_status();
        }
    } else if (_view_mode == DISPLAY_VIEW_DOWNLOAD) {
        _lcd_prog.refresh();
    } else if (_view_mode == DISPLAY_VIEW_INSTALL) {
        /* nothing for now */
    } else if (_view_mode == DISPLAY_VIEW_SELF_TEST) {
        _lcd_prog.refresh();
    }

    _cycle_count++;
}

void DisplayMan::self_test()
{
    uint32_t i;
    INDICATOR_TYPES indicators[] = {IND_POWER, IND_WIFI,  IND_CLOUD,
                                    IND_FWUP,  IND_LIGHT, IND_TEMP};

    for (i = 0; i < ARRAY_SIZE(indicators); i++) {
        led_set_color(indicators[i], IND_COLOR_IN_PROGRESS, false);
    }
    for (i = 0; i <= 90; i++) {
        _view_mode = DISPLAY_VIEW_SELF_TEST;
        set_progress("Self test", i, 90);
        refresh();
        Thread::wait(10);
    }
}
