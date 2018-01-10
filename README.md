# fota-demo

## Purpose

This is an mbed application for a sales tool that demonstrates firmware updates using the Firmware Over The Air (fota) capabilities of mbed and mbed Cloud 1.2. The tool contains environmental sensors for light temperature and humidity.  The sensor values are uploaded to the mbed cloud.  The sales tool is contained in a desktop plastic case.  It is battery powered and has LED indicators and an LCD display.
![photo](docs/photo.png)

## Getting fota-demo

mbed CLI can import the project, along with the mbed OS codebase and all dependent device drivers.

To import fota-demo, from the command line:

1. Navigate to a workspace directory of your choice.

	``cd ~/workspace``

2. Import the example:

	```
	git clone git@github.com:ARMmbed/fota-demo.git
	cd fota-demo
	```

    fota-demo is now under ``~/workspace/fota-demo``.  You can look at ``main.cpp`` to familiarize yourself with the code.

## Prerequisites

To build this project, you need to install the following:

1. arm-none-eabi-gcc version 6.3.1.20170215 or greater

Here is an example showing how to install on a Mac:
```
brew tap ARMmbed/homebrew-formulae
brew install arm-none-eabi-gcc
```

2. Python dependencies

```
	pip install -r requirements.txt
```

## Specifying a network configuration

The network configuration is specified in the project configuration file ``mbed_app.json``.  Open ``mbed_app.json`` and modify the following configuration values to suit the deployment environment:
```
    ...
    "wifi-ssid": {
        "help": "The SSID to connect to if using a WiFi interface",
        "value": "\"MYSSID\""
    },
    "wifi-security": {
        "help": "WPA, WPA2, WPA/WPA2, WEP, NONE, OPEN",
        "value": "\"WPA2\""
    },
    "wifi-password": {
        "help": "An optional password for wifi security authentication",
        "value": "\"MYPASSWORD\""
    }
    ...
```
The configuration can also be changed at runtime via a serial console.  See the section on serial commands for help with connecting to and using the console.  See the section on Wi-Fi Commissioning for the relevant keystore options.

## <a name="GetDevCert"></a>Downloading A Developer Certificate

A certificate is required for the end device to be able to communicate with mbed cloud.  Log on to the mbed cloud portal and navigate to ``Device Identity -> Certificates``.

If creating a new certificate, select the ``Actions`` pulldown and choose ``Create a developer certificate``.  Fill in the form and click ``Create Certificate``.  At this time, the certificate may be downloaded onto the local system.  Place the certificate C file in the root folder of the fota-demo project.

If downloading an existing certificate, click the name of the appropriate certificate from the list of certificates presented on the Certificates page.  Click ``Download Developer C file`` and place the certificate C file in the root folder of the fota-demo project.

## Compiling

The fota-demo project uses a Makefile to compile the source code.  The Makefile attempts to detect the toolchain and target and calls the mbed compiler with appropriate options.  To build for the current fota-demo hardware, you may need to set your mbed target to UBLOX_EVK_ODIN_W2.

**NOTE:** Previous versions of the fota-demo project were based on the K64F platform.  If you have hardware based on the K64F, then replace `UBLOX_EVK_ODIN_W2` with `K64F`.  If you do not know your platform, run the following commands in your build environment to give an indication:
```
$ mbedls
$ mbed detect
```
If the platform_name shown is `unknown`, use `UBLOX_EV_ODIN_W2` target.

**NOTE:** Versions prior to v1.7.0 no longer build due to outdated SHA references for the ws2801 and DHT libraries.
As a workaround, please update the corresponding .lib files with the following refs:
* ws2801 9706013b3a6aea3397320ba2383b9e2c924b64b8
* DHT f6cd0c6d7abdf3b570687f89839e0ca5e24c6b3f

Assuming you are compiling with GCC, your .mbed file should look like the following:

```
$ cat .mbed
ROOT=.
TARGET=UBLOX_EVK_ODIN_W2
TOOLCHAIN=GCC_ARM
```

If the file does not exist, you can either allow the Makefile to create it with default settings, or create it yourself with the following commands.
```
$ mbed config ROOT .
$ mbed target UBLOX_EVK_ODIN_W2
$ mbed toolchain GCC_ARM
```

Typing `make` builds the bootloader and app and combines them into a single image.  The final images are copied into the `bin/` folder.
```
$ make
```

### Compilation Errors

The fota-demo project will fail to compile if a developer certificate is not present in the local source directory.  This file is typically named ``mbed_cloud_dev_credentials.c`` and defines several key constants.  Please see [Downloading A Developer Certificate](#GetDevCert) for more information.

A missing certificate results in compilation errors similar to the following:
```
./BUILD/K64F/GCC_ARM/mbed-cloud-client-restricted/factory_client/factory_configurator_client/source/fcc_dev_flow.o: In function `fcc_developer_flow':
fcc_dev_flow.c:(.text.fcc_developer_flow+0x130): undefined reference to `MBED_CLOUD_DEV_BOOTSTRAP_ENDPOINT_NAME'
fcc_dev_flow.c:(.text.fcc_developer_flow+0x138): undefined reference to `MBED_CLOUD_DEV_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE'
fcc_dev_flow.c:(.text.fcc_developer_flow+0x13c): undefined reference to `MBED_CLOUD_DEV_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE_SIZE'
...
```

### Patching Errors

If the list of dependent libraries change, we will assume that mbed-os was updated as well and try to patch the linker scripts again.  This produces the following error:
```
error: patch failed: targets/TARGET_Freescale/TARGET_MCUXpresso_MCUS/TARGET_MCU_K64F/device/TOOLCHAIN_GCC_ARM/MK64FN1M0xxx12.ld:64
error: targets/TARGET_Freescale/TARGET_MCUXpresso_MCUS/TARGET_MCU_K64F/device/TOOLCHAIN_GCC_ARM/MK64FN1M0xxx12.ld: patch does not apply
```

If this happens, run `make distclean`, then `make`.

### Cleaning the build

```
make clean
```

Make clean will clean the c++ compile output.
```
make distclean
```
Make distclean will remove all dependency files and generated files.

## Flashing your board

The following command will copy `bin/combined.bin` to a USB-attached device.
```
make install
```

If this command fails or you have more than one device attached to your build system, you can manually copy the image to the device.  For example:
```
$ cp bin/combined.bin /Volumes/DAPLINK/
```
Make sure to substitute for the correct mount point of your device.

## Update over the air

```
make campaign
```

## Serial Command Help

A serial terminal can be connected to the device for the purpose of viewing diagnostic output and issuing serial commands.  Serial connection is a at a baud rate of 115200.

Press enter at any time to get a command prompt.

```
>
```

Typing `help` at the prompt provides a list of the commands and a brief set of usage instructions.

```
> help
Help:
del          - Delete a configuration option from the store. Usage: del <option>
get          - Get the value for the given configuration option. Usage: get [option] defaults to *=all
help         - Get help about the available commands.
reboot       - Reboot the device. Usage: reboot
reset        - Reset configuration options and/or certificates. Usage: reset [options|certs|all] defaults to options
set          - Set a configuration option to a the given value. Usage: set <option> <value>
```

### Option Keystore

The keystore is a simple name value pair database used to store configuration parameters, for example Wi-Fi credentials.

The following commands are provided to manipulate the keystore:

1. `get` get a key and print its value.


```
> get wifi.ssid

wifi.ssid=iotlab
```

2. `set` set a key to the given value.

```
> set wifi.ssid iotlab

wifi.ssid=iotlab
```

3. `del` delete a key and it's value.

```
> del wifi.ssid

Deleted key wifi.ssid
```

### Wi-Fi Commissioning

To configure Wi-Fi set the following key options:

```
> set wifi.ssid yourssid
wifi.ssid=yourssid

> set wifi.key passphrase
wifi.key=passphrase

> set wifi.encryption WPA2
wifi.encryption=WPA2
```

After setting the Wi-Fi credentials reset the device.

```
> reboot
```

### Reset

To delete all stored options and their values:

```
> reset           deletes the options keystore

> reset options   deletes the options keystore

> reset certs     deletes the fcc certs

> reset all       deletes fcc certs and options keystore
```


## M2M Resources

The fota-demo firmware exposes several M2M resource IDs.  Most of these resources are read-only sensor measurements.

The fota-demo firmware also exposes the following resources:

### Application Info

* Object ID: 26241

#### Application Label

* Resource ID: 1
* Path: /26241/0/1

The app label is a user-friendly name that can be read and written.  The app label is displayed on the LCD with a prefix of ``Label: ``.

The app label can be written in 3 ways:
1. By setting `app-label` in the config section of ``mbed_app.json``.

    ```
        "config": {
            "app-label": {
                "help": "Sets a device friendly name displayed on the LCD",
                "value": "\"dragonfly\""
            },
        ...
    ```

2. Through M2M PUT requests
This can be demonstrated on the mbed cloud portal.  After a device is registered with the mbed cloud, it should be listed on the ``Connected Devices`` page.  Click on the Device ID to bring up device details and then click on the ``Resources`` tab.  Scroll to ``Object ID 26241`` and click ``/26241/0/1``.  On the popup dialog, click ``Edit`` and enter a new name in the ``Value`` text box.  Make sure that the ``PUT`` request type is chosen and then click the ``Send`` button.

3. Through the serial console by setting the `app.label` key.

    ```
    > set app.label anisoptera
    app.label=anisoptera
    ```

    Note when changing the app label via the serial console, a reboot must be performed in order for the setting to take effect.

#### Application Version

* Resource ID: 2
* Path: /26241/0/2

The app version is read-only.  Its value is populated from the "version" field in ``mbed_app.json``.

### User Configured Geographical Info

* Object ID: 3336
* Instance: 0

Latitude, Longitude, and Accuracy info can be stored on the device and made available via M2M in instance 0 of object ID 3336.  Along with lat/long, a resource of named ``Application Type`` is also present, and is set to ``user`` in the case of instance 0.  This is to allow for other types of geographical data to be made available on a separate instance which can be differentiated by a type.

Geographical data can be written in 3 ways:
1. By setting the following keys in the `config` section of `mbed_app.json`.

    ```
        "config": {
            "geo-lat": {
                "help": "Sets the device latitude, from -90.0 to 90",
                "value": "\"30.2433\""
            },
            "geo-long": {
                "help": "Sets the device longitude, from -180 to 180",
                "value": "\"-97.8456\""
            },
            "geo-accuracy": {
                "help": "Sets the accuracy of geo-lat and geo-lon, in meters",
                "value": "\"11\""
            },
        ...
    ```

2. Through the serial console by setting the following keys:

    ```
    > set geo.lat 30.2433
    geo.lat=30.2433

    > set geo.long -97.8456
    geo.long=-97.8456

    > set geo.accuracy 11
    geo.accuracy=11
    ```

3. Through M2M PUT requests

    This can be demonstrated on the mbed cloud portal.  After a device is registered with the mbed cloud, it should be listed on the ``Connected Devices`` page.  Click on the Device ID to bring up device details and then click on the ``Resources`` tab.  Scroll to ``Object ID 3336`` and click on resources attached to instance 0, such as ``/3336/0/5514`` which shows latitude, ``/3336/0/5515`` which shows longitude, and ``/3336/0/5516`` which shows accuracy.  On the popup dialog, click ``Edit`` and enter a new value in the ``Value`` text box.  Make sure that the ``PUT`` request type is chosen and then click the ``Send`` button.

Here are some additional details about each of the user-configurable geographical resources.  Unless otherwise specified, each resource has a keystore option which can be modified on the serial console.  Please see the section entitled Option Keystore for more details about how to use this interface.

Geographical data can be effectively deleted from the mbed cloud portal by setting the resource to a '-' (dash).  This special character allows upstream web apps to clean up state and make any appropriate changes.

    ```
    > set geo.lat -
    geo.lat=-

    > set geo.long -
    geo.long=-

    > set geo.accuracy -
    geo.accuracy=-

    > reboot
    ```

####  Type

* Resource ID: 5750
* Path: /3336/0/5750
* option key: none - this resource is not configurable
* value: "user"

#### Latitude

* Resource ID: 5514
* Path: /3336/0/5514
* option key: geo.lat

#### Longitude

* Resource ID: 5515
* Path: /3336/0/5515
* option key: geo.long

#### Accuracy

* Resource ID: 5516
* Path: /3336/0/5516
* option key: geo.accuracy
