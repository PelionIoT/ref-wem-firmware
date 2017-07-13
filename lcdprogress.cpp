#include "lcdprogress.h"

LCDProgress::LCDProgress(MultiAddrLCD &lcd) : _lcd(lcd)
{
    char backslash[] = {0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00, 0x00};
    lcd.setUDC(7, backslash);
    uint8_t pixel_row[] = {0x0, 0x10, 0x18, 0x1C, 0x1E};
    for (int i = 1; i < 5; i++) {
        char pixels[8];
        for (int j = 0; j < 8; j++) {
            pixels[j] = pixel_row[i];
        }
        _lcd.setUDC(i, pixels);
    }
}

void LCDProgress::set_progress(const char *message, uint32_t progress,
                               uint32_t total)
{
    static int spinner_counter = 0;
    char spinner;
    char progressbar[17];
    uint32_t i, lines, bars;

    switch (spinner_counter) {
        case 0:
            spinner = '-';
            break;
        case 1:
            spinner = '/';
            break;
        case 2:
            spinner = '|';
            break;
        case 3:
            spinner = 7;
            break;
        default:
            spinner = '?';
            break;
    }

    /* calculate progress bar */
    lines = progress * 16 * 5 / total;
    bars = lines / 5;
    for (i = 0; i < bars; i++) {
        progressbar[i] = 0xff; // Full bar
    }
    if (i < 16 && lines > bars * 5) {
        uint8_t partial_columns = lines % 5;
        progressbar[i] = partial_columns;
        i++;
    }
    for (; i < 16; i++) {
        progressbar[i] = ' ';
    }
    progressbar[16] = 0;

    _lcd.printline(0, "%-15s%c", message, spinner);
    _lcd.printline(1, "%s", progressbar);

    spinner_counter = (spinner_counter + 1) % 4;
}

