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


MultiAddrLCD::MultiAddrLCD(I2C *i2c) : _lcd1(i2c, 0x4e, TextLCD::LCD16x2, TextLCD::HD44780),
    _lcd2(i2c, 0x7e, TextLCD::LCD16x2, TextLCD::HD44780) {
}

/*Only supporting 16x2 LCDs, so string will be truncated at 32
  characters.*/
int MultiAddrLCD::printf(const char* format, ...) {
    int rc;
    char buf[33];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, 32, format, args);
    va_end(args);
    _lcd1.printf(buf);
    rc = _lcd2.printf(buf);
    return rc;
}

/*Only supporting 16x2 LCDs, so string will be truncated at 16
  characters.*/
int MultiAddrLCD::printline(int line, const char* format, ...) {
    int rc;
    char buf[17];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, 16, format, args);
    va_end(args);
    _lcd1.locate(0, line);
    _lcd1.printf("%16s", buf);
    _lcd2.locate(0,line);
    rc = _lcd2.printf("%16s", buf);
    return rc;
}

void MultiAddrLCD::setBacklight(TextLCD_Base::LCDBacklight mode) {
    _lcd1.setBacklight(mode);
    _lcd2.setBacklight(mode);
}


void MultiAddrLCD::setCursor(TextLCD_Base::LCDCursor mode) {
    _lcd1.setCursor(mode);
    _lcd2.setCursor(mode);
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
    _lcd.printline(0, "Version: %s", version);
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

void DisplayMan::set_network_in_progress() {
    _network_sensor_id = this->register_sensor("Wifi");
    led_set_color(IND_WIFI, IND_COLOR_IN_PROGRESS, true);
}

void DisplayMan::set_network_fail() {
    led_set_color(IND_WIFI, IND_COLOR_FAILED);
    _lcd.printline(1, "Fail: %s", MBED_CONF_APP_WIFI_SSID);
}

void DisplayMan::set_network_success() {
    led_set_color(IND_WIFI, IND_COLOR_SUCCESS);
    this->set_sensor_status(_network_sensor_id, "connected");
}

int DisplayMan::register_sensor(const char *name) {
    _sensors.push_back(SensorDisplay());
    _sensors[_next_sensor_id].name.assign(name);
    _sensors[_next_sensor_id].status.assign("");
    return _next_sensor_id++;
}

void DisplayMan::set_sensor_status(int sensor_id, char *status) {
    if (sensor_id < _sensors.size()) {
        _sensors[sensor_id].status.assign(status);
    }
}

void DisplayMan::cycle_status() {
    char line[17];
    snprintf(line, 16, "%s: %s", _sensors[_active_sensor].name.c_str(),
             _sensors[_active_sensor].status.c_str());
    _lcd.printline(1, line);
    _active_sensor = (_active_sensor + 1) % _sensors.size();
}
