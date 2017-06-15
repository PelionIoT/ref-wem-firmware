## Hardware

This device demonstrates features of Mbed Cloud, including firmware updates and sensor data uploads. It consists of 13 off-the-shelf parts, a 3D printed enclosure, and costs roughly US $110 to make. You can 3D print the enclosure or manufacture it in other ways. If you 3D print the enclosure, you must sand and paint it to give it a nice finished look. The enclosure is a top and bottom part that lock together.

### Parts

Buy the following off-the-shelf items:

- [Main board FRDM-K64F](https://developer.mbed.org/platforms/FRDM-K64F/).
    - [Buy the FRDM-K64F](http://www.mouser.com/ProductDetail/NXP/FRDM-K64F/).
- [Base Shield V2](https://www.seeedstudio.com/Base%20Shield%20V2-p-1378.html).
- [A micro SD card for storage](https://www.sandisk.com/home/memory-cards/microsd-cards/sandisk-microsd).
    - [Buy the card](https://www.amazon.com/SanDisk-Memory-Frustration-Free-Packaging-SDSDQ-008G-AFFP/dp/B007KFXICK).
- [Wi-Fi ESP8266](https://www.seeedstudio.com/Grove-Uart-Wifi-p-2495.html).
- [LED WS2801 string (7x LEDs)](https://cdn-shop.adafruit.com/datasheets/WS2801.pdf).
    - [Buy the string](https://www.amazon.com/WS2801-Module-Without-individually-Addressable/dp/B0192VUDNG/ref=sr_1_2).
- LCD 16x2 I2C character display.
    - [Buy the display](https://www.amazon.com/gp/product/B01LC7ECAS).
    - Information about the display:
        - [LCD DM-LCD1602-402](https://drive.google.com/file/d/0B5lkVYnewKTGalhWazNhOGxVbUE/view?usp=sharing).
        - [Display driver SPLC780D](https://drive.google.com/file/d/0BxCL-uXywP6wQXlvMnRIaFN6UVU/view?usp=sharing).
        - [Product information](https://www.displaymodule.com/products/dm-lcd1602-402).
        - [More product information](https://www.amazon.com/Qunqi-Serial-Backlight-Arduino-MEGA2560/dp/B01E4YUT3K).
    - Note: This component requires the Grove Shield to be set to VCC=5V.
- Low profile angled micro USB cable.
    - [Buy the cable](https://www.amazon.com/CablesOnline-Micro-B-Position-Extension-AD-U44/dp/B00JSXUJ7Y/).
    - [Buy the cable](https://www.amazon.com/gp/product/B01LZFKVDP).
- [Filter Thermwell 15X24x3/16 open cell foam F1524 air conditioner filter](https://www.amazon.com/gp/product/B000BO68BU).
- [Grove Light Sensor (P) v1.1](https://www.seeedstudio.com/Grove-Light-Sensor-%28P%29-v1.1-p-2693.html).
- [Grove Temperature&Humidity Sensor Pro](https://www.seeedstudio.com/grove-temperaturehumidity-sensor-pro-p-838.html).
- [Battery (LiPo 3.7V/2500mAh)](https://www.adafruit.com/product/328).
- [Power charger (5V/1A LiPo USB)](https://www.adafruit.com/product/2465).
- [Power switch (1P2T SPDT slide switch)](https://www.amazon.com/gp/product/B007QAJMHO).

This document describes how the above parts connect to build the working device.

### Install DAPLink onto the main board

If you are using a new K64 board, it likely does not have DAPLink on it. DAPLink makes flashing firmware onto it much easier.

1. Download https://blackstoneengineering.github.io/DAPLink//firmware/0243_k20dx_frdmk64f_0x5000.bin.
1. On the K64 board, hold down the reset button, and plug it into your computer. It appears as 'BOOTLOADER'.
1. Drag and drop the DAPLink firmware update to the `BOOTLOADER` USB drive.
1. Wait for the firmware to finish saving.
1. Reset the board by power cycling it. The board re-enumerates as `DAPLink`.

You can find more details and instructions at https://blackstoneengineering.github.io/DAPLink/.

### Modify LEDs

![](led_wires.jpg)

You must modify the wires on the LED to connect to the Base Shield. The LEDs have four wires:

- Ground (GND), blue, must connect to pin `GND` on the base shield.
- Clock (CO), green, must connect to pin `D3` on the base shield.
- Data (DO), white, must connect to pin `D2` on the base shield.
- Power (+5V), red, must connect to pin `VCC` on the base shield.

On the base shield, the slot "D2" has those four pins: `GND`, `VCC`, `D3` and `D2`.

### Adjust the LCD

![](lcd.jpg)

1. Put a jumper on the two pins next to the `LED` label to turn on the bright LED light.
1. You may have to slightly adjust the screw in the blue box to adjust the brightness of the display. If it is too dim, you won't be able to read the words.

### Basic connections

![](basic_connections.jpg)

1. Plug the SD card into the main board.
1. Connect the base shield to the top of the main board.
1. Plug the light sensor into port `A0` on the base shield.
1. Plug the temperature and humidity sensor into `D4` on the base shield.
1. Plug the Wi-Fi into `UART` on the base shield.
1. Plug the LCD into `I2C` on the base shield.
1. Plug the LED lights into `D2` on the base shield.

### The case

![](case_parts.jpg)

You can 3D print the case using files at https://github.com/ARMmbed/fota-hardware.

There are seven `STL` files in the directory `CAD/STL/` that 3D printers or other manufacturing processes can use to create those parts. (If you wish to edit the design, you need to use SolidWorks software to modify the files in the `CAD` directory.)

#### If using a 3D printer

Open your printer software, and then:

1. Open two files together: `Enclosure top.stl` and `Enclosure light pipes.stl`. Right-click, and select `merge`. These two parts are printed with different materials. The top is opaque, and the light pipes are transparent to allow light to shine through.
1. Select `print`.
1. Wait several hours!

You can print the remaining five STL files separately. The body takes the longest amount of time to print. Print the externally visible parts (top, middle and bottom) using fine-grained settings to ensure as smooth a finish as possible.

#### Manufacturing choices

The top part has built-in light channels, so manufacturing choices are split in two categories:

1. Printing the top part without light channels and adding them in-house, either with translucent material pouring (for example, epoxy or silicone gel), or with press-fitting acrylic cut parts. Both variants are difficult and are likely to yield low-quality results, such as uneven surfaces or gaps.
1. Printing the top part with a dual material process. This is the recommended action, even though it comes with a severe limitation in manufacturing processes.

Only fused deposition modeling (FDM) and PolyJet can embed translucent material into a part. Of the two, FDM is the most cost effective, and PolyJet makes better looking results. The exact difference in unfinished quality varies with the manufacturer. The recommended course of action is to create one unit with each of the two processes and pick one to move forward with based on visual inspection.

#### Finishing choices

Manual sanding and painting leads to the best results. Manufacturing houses all want to avoid this because of the high and nonscalable cost. Some houses offer automatic sanding, and some mention the ability to paint but without specific pricing. The recommendation is to manufacture the parts with no finish or with only automatic sanding and determine whether the outcome looks satisfactory.

### Gluing LEDs into the case

![](leds.jpg)

The piece created by the file `Enclosure LED-round plate.stl` holds the LEDs. Super-glue the LEDs into the plate in order from closest (1) to farthest (7) from the board.

### Power switch

![](power.jpg)

This contains a switch that allows the user to choose either battery or USB power. The USB wire has four wires that you must solder to the correct place: white to pin 2, green to pin 3, black to ground, red to +5 V.

### Bottom with battery and power

![](battery.jpg)

The bottom holds the battery, power switch and USB charger.

### Top with displays

![](leds_ready.jpg)
![](lcd_display.jpg)

1. Put the LCD display into the top first. Use hot glue in the corners to keep it in place.
1. Push the two LED holders into the top.

### Middle with main board and sensors

![](middle.jpg)

1. Verify the switch on the shield is set to 5V. This is required. Use hot glue to keep it in place.
1. While being careful with the SD card, push the main board down into the middle part of the case (the `Enclosure body`).
1. Push the two sensor mounts (the `Enclosure sensor mounts LHS` and `Enclosure sensor mounts RHS`) down next to main board. They clip in). You can also glue them
1. Place the sensors into the sensor mounts.

### Put together the top, middle and bottom

1. Connect the USB cable from the case bottom to the main board.
1. Connect the LED and LCD cables from the case top to the main board.
1. While keeping all cables inside the case, screw the bottom and the top onto the middle part of the case.
1. Power it on (by turning power switch to use battery), and the device boots:

![](photo.png)

### Test normal start up

1. When the power is on, a GREEN LED lights up the battery icon.
1. The LCD displays the device version.
1. The Wi-Fi LED flashes YELLOW as it connects.
1. The LCD displays the SSID of the Wi-Fi it connects to.
1. The Wi-Fi LED turns a solid BLUE when the board successfully connects to Wi-Fi.
1. The board then connects to Mbed Cloud, and the cloud LED flashes YELLOW.
1. The LCD dispalys reasonable sensor data, such as temperature, humidity and light.
1. The cloud LED turns solid BLUE when the board connects and registers to Mbed Cloud.
1. As the device uploads sensor data, the LEDs for the sensors and cloud flash GREEN.
