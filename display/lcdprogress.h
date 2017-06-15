// ****************************************************************************
//  Workplace Environmental Monitor
//
//  This is a reference deployment application utilizing mbed cloud 1.2.
//
//  By the ARM Reference Design Team
// ****************************************************************************

#ifndef __LCDPROGRESS_H_
#define __LCDPROGRESS_H_

#include "multiaddrlcd.h"

#include <string>

class LCDProgress {
public:
    LCDProgress(MultiAddrLCD &lcd);
    void set_progress(const std::string &message, uint32_t progress,
                      uint32_t total);
    void refresh();
    void reset();

private:
    MultiAddrLCD &_lcd;
    std::string _buffer;
    std::string _previous;
};

#endif
