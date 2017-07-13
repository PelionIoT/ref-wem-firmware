#include "multiaddrlcd.h"

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
    vsnprintf(buf, 33, format, args);
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
    vsnprintf(buf, 17, format, args);
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

void MultiAddrLCD::setUDC(unsigned char c, char *udc_data) {
    _lcd1.setUDC(c, udc_data);
    _lcd2.setUDC(c, udc_data);
}
