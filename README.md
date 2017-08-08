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
brew tap https://github.com/ARMmbed/homebrew-formulae
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

The fota-demo project uses a Makefile to compile the source code.  The Makefile attempts to detect the toolchain and target and calls the mbed compiler with appropriate options.

```
make
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

```
make install
```

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

The app label is a user-friendly name that can be read and written.  The app label is displayed on the LCD with a prefix of ``Name: ``.

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
