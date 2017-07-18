#include "displayman.h"

void thread_display_update(DisplayMan *display) {
    while (true) {
        for (int i=0; i<20; i++) {
            led_post();
            Thread::wait(100);
        }
        //For now, page through the status of all sensors
        display->cycle_status();
    }
}

DisplayMan::DisplayMan() : _i2c(I2C_SDA, I2C_SCL), _lcd(&_i2c) {
    _next_sensor_id = 0;
}

int DisplayMan::init() {
    led_setup();
    _lcd.setBacklight(TextLCD_I2C::LightOn);
    _lcd.setCursor(TextLCD_I2C::CurOff_BlkOff);
    return 0;
}

void DisplayMan::set_power_on() {
    led_set_color(IND_POWER, IND_COLOR_ON);
}

void DisplayMan::set_version_string(const char *version) {
    _version_string = version;
}

void DisplayMan::set_cloud_in_progress() {
    led_set_color(IND_CLOUD, IND_COLOR_IN_PROGRESS, true);
}


void DisplayMan::set_cloud_registered() {
    led_set_color(IND_CLOUD, IND_COLOR_SUCCESS);
}

void DisplayMan::set_cloud_unregistered() {
    led_set_color(IND_CLOUD, IND_COLOR_OFF);
}

void DisplayMan::set_cloud_error() {
    led_set_color(IND_CLOUD, IND_COLOR_FAILED);
}

void DisplayMan::init_network(const char *type) {
    if (0 == _network_sensor_id) {
        _network_sensor_id = register_sensor(type);
    } else {
        set_sensor_name(_network_sensor_id, type);
    }
}

void DisplayMan::set_network_in_progress() {
    led_set_color(IND_WIFI, IND_COLOR_IN_PROGRESS, true);
    set_sensor_status(_network_sensor_id, MBED_CONF_APP_WIFI_SSID);
}

void DisplayMan::set_network_fail() {
    led_set_color(IND_WIFI, IND_COLOR_FAILED);
    set_sensor_status(_network_sensor_id, MBED_CONF_APP_WIFI_SSID);
}

void DisplayMan::set_network_success() {
    led_set_color(IND_WIFI, IND_COLOR_SUCCESS);
    set_sensor_status(_network_sensor_id, MBED_CONF_APP_WIFI_SSID);
}

uint8_t DisplayMan::register_sensor(const char *name) {
    _sensors.push_back(SensorDisplay());
    _sensors[_next_sensor_id].name.assign(name);
    _sensors[_next_sensor_id].status.assign("");
    return _next_sensor_id++;
}

void DisplayMan::set_sensor_status(uint8_t sensor_id, const char *status) {
    if (sensor_id < _sensors.size()) {
        _sensors[sensor_id].status.assign(status);
    }
}

void DisplayMan::set_sensor_name(uint8_t sensor_id, const char *name) {
    if (sensor_id < _sensors.size()) {
        _sensors[sensor_id].name.assign(name);
    }
}

void DisplayMan::cycle_status() {
    char line[17];

    /* top line */
    _lcd.printline(0, "Version: %s", _version_string.c_str());

    /* bottom line */
    snprintf(line, 16, "%s: %s", _sensors[_active_sensor].name.c_str(),
             _sensors[_active_sensor].status.c_str());
    _lcd.printline(1, line);
    _active_sensor = (_active_sensor + 1) % _sensors.size();
}

MultiAddrLCD& DisplayMan::get_lcd() {
    return _lcd;
}
