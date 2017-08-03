# Hardware

## Parts

Order the following off-the-shelf items, total cost is roughly 110 USD:

1. Main board FRDM-K64F
   * Buy: http://www.mouser.com/ProductDetail/NXP/FRDM-K64F/
   * Info: https://developer.mbed.org/platforms/FRDM-K64F/
1. Base Shield V2
   * Info/Buy: https://www.seeedstudio.com/Base%20Shield%20V2-p-1378.html
1. Storage, MicroSD card
   * Buy: https://www.amazon.com/SanDisk-Memory-Frustration-Free-Packaging-SDSDQ-008G-AFFP/dp/B007KFXICK
   * Info: https://www.sandisk.com/home/memory-cards/microsd-cards/sandisk-microsd
1. WiFi ESP8266
   * Info/Buy: https://www.seeedstudio.com/Grove-Uart-Wifi-p-2495.html
1. LED  WS2801 string (7x LEDs)
   * Buy: https://www.amazon.com/WS2801-Module-Without-individually-Addressable/dp/B0192VUDNG/ref=sr_1_2
   * Info: https://cdn-shop.adafruit.com/datasheets/WS2801.pdf
1. LCD  16x2 I2C character display
   * Buy: https://www.amazon.com/gp/product/B01LC7ECAS
   * Info:
     * LCD DM-LCD1602-402: https://drive.google.com/file/d/0B5lkVYnewKTGalhWazNhOGxVbUE/view?usp=sharing
     * display driver SPLC780D: https://drive.google.com/file/d/0BxCL-uXywP6wQXlvMnRIaFN6UVU/view?usp=sharing
     * cross ref product info: https://www.displaymodule.com/products/dm-lcd1602-402
     * cross ref product info: https://www.amazon.com/Qunqi-Serial-Backlight-Arduino-MEGA2560/dp/B01E4YUT3K
   * Note: This component requires the Grove Shield to be set to VCC=5V
1. Low profile angled microUSB cable
   * Buy: https://www.amazon.com/CablesOnline-Micro-B-Position-Extension-AD-U44/dp/B00JSXUJ7Y/
   * Buy: https://www.amazon.com/gp/product/B01LZFKVDP
1. Filter Thermwell 15X24x3/16 Open Cell Foam F1524 Air Conditioner Filter
   * Buy: https://www.amazon.com/gp/product/B000BO68BU
1. Grove Light Sensor (P) v1.1
   * Info/Buy: https://www.seeedstudio.com/Grove-Light-Sensor-%28P%29-v1.1-p-2693.html
1. Grove Temperature&Humidity Sensor Pro
   * Info/Buy: https://www.seeedstudio.com/grove-temperaturehumidity-sensor-pro-p-838.html

## Modify LEDs

![](led_wires.jpg)

The wires on the LED must be modified to connect to the Base Shield. The LEDs have 4 wires:

1. Ground (GND), blue, must connect to pin "GND" on the base shield.
1. Clock (CO), green, must connect to pin "D3" on the base shield.
1. Data (DO), white, must connect to pin "D2" on the base shield.
1. Power (+5V), red, must connect to pin "VCC" on the base shield.

On the base shield, the slot "D2" has those 4 pins: "GND", "VCC", "D3", and "D2".

## Adjust LCD

![](lcd.jpg)

1. Put a jumper on the two pins next to the "LED" label. This is needed to turn on the bright led light.
1. You may have to slightly adjust the screw in the blue box to adjust the brightness of the display. If it is too dim, people won't be able to read the words.

## Basic Connections

![](basic_connections.jpg)

1. Plug the SD Card into the Main Board.
1. Connect the "Base Shield" to the top of the Main Board.
1. Plug the light sensor into port "A0" on the Base Shield.
1. Plug the Temp/Humidity sensor into "D4" on the Base Shield.
1. Plug the Wifi into "UART" on the Base Shield.
1. Plug the LCD into "I2C" on the Base Shield.
1. Plug the LED lights into "D2" on the Base Shield.

## The Case

The case can be 3D Printed using CAD files at https://github.com/ARMmbed/fota-hardware

The case consists of 3 parts:

1. A top which holds LEDs and the LCD display.
1. A bottom which holds batteries.
1. A mid section which holds the base board and most parts.

