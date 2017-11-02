#include "multiaddrlcd.h"

#if TARGET_UBLOX_EVK_ODIN_W2
MultiAddrLCD::MultiAddrLCD(PinName rs, PinName e, PinName d4, PinName d5, PinName d6, PinName d7)
    : _lcd1(rs, e, d4, d5, d6, d7)
#else
MultiAddrLCD::MultiAddrLCD(I2C *i2c)
    : _lcd1(i2c, 0x4e, TextLCD::LCD16x2, TextLCD::HD44780),
      _lcd2(i2c, 0x7e, TextLCD::LCD16x2, TextLCD::HD44780)
#endif
{
}

/*Only supporting 16x2 LCDs, so string will be truncated at 32
  characters.*/
int MultiAddrLCD::printf(const char *format, ...)
{
    int rc;
    char buf[33];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    rc = _lcd1.printf(buf);
#if !TARGET_UBLOX_EVK_ODIN_W2
    _lcd2.printf(buf);
#endif
    rc = _lcd1.printf(buf);

    return rc;
}

/*Only supporting 16x2 LCDs, so string will be truncated at 16
  characters.*/
int MultiAddrLCD::printline(int line, const char *msg)
{
    int rc;
    char buf[17];
    snprintf(buf, sizeof(buf), "%s", msg);
#if !TARGET_UBLOX_EVK_ODIN_W2
    _lcd2.locate(0, line);
    _lcd2.printf("%-16s", buf);
#endif
    _lcd1.locate(0, line);
    rc = _lcd1.printf("%-16s", buf);

    return rc;
}

int MultiAddrLCD::printlinef(int line, const char *format, ...)
{
    int rc;
    char buf[17];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
#if !TARGET_UBLOX_EVK_ODIN_W2
    _lcd2.locate(0, line);
    _lcd2.printf("%-16s", buf);
#endif
    _lcd1.locate(0, line);
    rc = _lcd1.printf("%-16s", buf);

    return rc;
}

void MultiAddrLCD::setBacklight(TextLCD_Base::LCDBacklight mode)
{
    _lcd1.setBacklight(mode);
#if !TARGET_UBLOX_EVK_ODIN_W2
    _lcd2.setBacklight(mode);
#endif
}

void MultiAddrLCD::setCursor(TextLCD_Base::LCDCursor mode)
{
    _lcd1.setCursor(mode);
#if !TARGET_UBLOX_EVK_ODIN_W2
    _lcd2.setCursor(mode);
#endif
}

void MultiAddrLCD::setUDC(unsigned char c, char *udc_data)
{
    _lcd1.setUDC(c, udc_data);
#if !TARGET_UBLOX_EVK_ODIN_W2
    _lcd2.setUDC(c, udc_data);
#endif
}

void MultiAddrLCD::locate(int column, int row)
{
    _lcd1.locate(column, row);
#if !TARGET_UBLOX_EVK_ODIN_W2
    _lcd2.locate(column, row);
#endif
}

void MultiAddrLCD::putc(int c)
{
    _lcd1.putc(c);
#if !TARGET_UBLOX_EVK_ODIN_W2
    _lcd2.putc(c);
#endif
}
