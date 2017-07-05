# fota-demo

## Purpose

This is an mbed application for a sales tool that demonstrates firmware updates using the Firmware Over The Air (fota) capabilities of mbed and mbed Cloud 1.2.

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

	```
	pip install -r requirements.txt
	```

## Setting up mbed environment

Need to make the current directory an mbed project and install the mbed-os library for the first time setup.

```
mbed config root .
mbed deploy
```

## Specifying a network configuration

The network configuration is hard-coded into the project configuration file ``mbed_app.json``.

Open ``mbed_app.json`` and modify the following configuration values to suit the deployment environment:

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
