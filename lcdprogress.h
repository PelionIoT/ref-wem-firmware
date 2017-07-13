// ****************************************************************************
//  Firmware Over The Air (FOTA) demo
//
//  This application demonstrates how to perform fota using mbed cloud 1.2.
//
//  By the ARM Reference Design (Red) Team
// ****************************************************************************

#ifndef __LCDPROGRESS_H_
#define __LCDPROGRESS_H_

#include "multiaddrlcd.h"

class LCDProgress
{
  public:
    LCDProgress(MultiAddrLCD &lcd);
    void set_progress(const char *message, uint32_t progress, uint32_t total);

  private:
    MultiAddrLCD &_lcd;
};

#endif
