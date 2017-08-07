# Software

## Requirements

In order to proceed with the setup for building the FOTA Demo firmware the following are needed:

1. A computer which can be used to host the GCC toolchain and environment necessary to build the firmware.
    * Operating System & Architectures supported:
      * Windows 7 or 10 (x86 or x86-64)
      * Linux (x86-64)
      * Mac OS X (x86-64)
    * USB port is required along with a type B micro USB cable to flash the K64F.
1. An internet connection to download the necessary software to build the firmware.
1. An mBed Cloud 1.2 Account (does not work with previous versions of mBed Cloud)
1. Basic knowledge of executing commands from a shell or command prompt.
1. The following utilties:
    * Python2 (NOTE: Python3 is not supported)
    * Make, or equivalent utility, to execute a Makefile to build the target.
    * Bash, or another shell equivalent
    * cat
    * awk
    * grep
    * sed
    * git
    * ln
    * cp
    * ctags
    * rm
    * manifest tool
        * NOTE: This can be installed with "pip install git+ssh://git@github.com/ARMmbed/manifest-tool-restricted.git@v1.2rc2"
    * echo
    * touch

## Setup

1. <a id="setup-workspace"/>Create a workspace directory for hosting the toolchain environment and building the firmware.
1. <a id="setup-toolchain"/>Download the appropriate toolchain here:
    * [Windows](https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/6-2017q2/gcc-am-none-eabi-6-2017-q2-update-win32-sha2.exe)
    * [Linux](https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2)
    * [Mac OS X](https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-mac.tar.bz2)
1. <a id="setup-mbed-cli"/>Setup and install mbed-cli using the following instructions: [mbed-cli Setup](https://github.com/ARMmbed/mbed-cli#installing-mbed-cli)
1. <a id="setup-cloud-key"/>Obtain your mBed Cloud key.
    1. Log in to the [mBed Cloud Portal](https://portal.us-east-1.mbedcloud.com/login) with your username and password.
    1. Click on 'Manage access' then 'API keys' and then  select 'Create new API key'.
    1. Give a name to your API key and from the Group drop down box select 'Developers' and then hit create.
    1. On the next screen you'll see the name you gave your key and the API key itself. Save the key to a file named ***mbed-cloud-key.txt***. This will be used later on in the [instructions](#ins-cloud-key).
1. <a id="setup-cloud-cert"/>Obtain your mBed Cloud certificate.
    1. Log in to the [mBed Cloud Portal](https://portal.us-east-1.mbedcloud.com/login) with your username and password.
    1. Click on 'Device Identity' then 'Certificates'; from the Actions drop down menu select 'Create developer certificate'.
    1. Give your certificate a name and description (optional) and then hit 'Create certificate'.
    1. You should be redirected back to the 'Certificates' page if the certificate was successfully created. If you click on the name of the certificate in the window a popup should appear which has a button to download the certificate as a source file; go ahead and hit 'Download Developer C file'. This will be used later on in the [instructions](#ins-cloud-cert).

## Instructions

1. Open up a shell or command prompt and change directories to the workspace you created in [Setup](#setup-workspace).
1. <a id="ins-import-fota"/>Grab the current version of the firmware using mbed-cli:
    ``` bash
    mbed import fota-demo
    ```
1. <a id="ins-cloud-key"/>Change directories into the fota-demo directory and copy the mbed-cloud-key.txt file created in [Setup](#setup-cloud-key) to ***.mbed-cloud-key***
    * NOTE: There is a dot in front of the name and no extension on the filename
1. <a id="ins-cloud-cert"/>Now copy in the ***mbed_cloud_dev_credentials.c*** source file that was downloaded in the [Setup](#setup-cloud-cert) steps to fota-demo directory.
1. Next setup the mbed environment to find your toolchain by issuing the following command:
   ```bash
   mbed config -G GCC_ARM_PATH <workspace>/<gcc-toolchain-dir>/bin
   ```
    * NOTE: The path should be pointing to the bin directory where you extracted the gcc toolchain.
1. Using a text editor open the ***mbed_app.json*** file and change the WiFi SSID, Security Type, and Password fields to connect to a nearby AP with an internet connection.
    * NOTE: Ensure that the "value" field under the "wifi" key is set to true to enable WiFi.
    * NOTE: If connecting to an OPEN network with no security you do not need to set the password.
1. Once setup you can issue the make command to build the firmware.
   ```bash
   make
   ```
1. <a id="ins-fw-loc"/>The resulting firmware image should be in BUILD/K64F/GCC_ARM/combined.bin
    * NOTE: The build directory may vary if building for a different target. The syntax for output directory is BUILD/\<target>/\<toolchain>

## Testing

1. Follow the mBed Handbook instructions for setting up the [PC Configuration](https://developer.mbed.org/platforms/FRDM-K64F/#pc-configuration).
1. Next follow the instructions for [Flashing a project binary](https://developer.mbed.org/platforms/FRDM-K64F/#flash-a-project-binary).
    * NOTE: Make sure the binary you copy over on to the board is ***combined.bin*** image created in the [Instructions](#ins-fw-loc) to the mounted drive.
1. Once to image has been copied over remove power from the device and then power it back on.
1. When the device is powered on you should see the power LED has turned GREEN.
    1. When the device is connecting to the WiFi station it will blink YELLOW. Once it has successfully connected it will turn solid BLUE. If it fails to connect it will turn solid RED.
    1. When the device is connecting to mBed Cloud the LED will blink YELLOW. Once it hass successfully connected to mBed cloud it will turn solid BLUE. If it fails to connect it will turn solid RED.
    1. Version information will display on the top line of the LCD. Environmental data and network status will display on the bottom line.
    1. Data being sent to the cloud will be illuminated by the colored LEDs. The solid BLUE LEDs will change to CYAN briefly and back to solid BLUE for every time environmental data is sent to mBed Cloud.

## Example (FOTA)

1. Make sure your device is powered on and connected to mBed Cloud.
1. <a id="example-edit-version"/>In your workspace directory where the fota-demo directory was [checked out](#mbed-import-fota) open up the ***mbed_app.json*** file and find the "version" key in the json file.
1. Change the "value" key underneath the version to a new number or string.
1. Open a shell and change directories to the fota-demo directory and execute the following command
    ```bash
    make && make campaign
    ```
    This command will rebuild your application. On a successful build it will upload the new image, then create a new manifest and campaign that will be sent to the cloud using the credentials stored in ***.mbed-cloud-key*** created in the [instructions](#setup-cloud-key)
1. Once the manifest and campaign have been uploaded you should see the LCD on the device display "Downloading..." and the firmware indicator should be blinking YELLOW.
    * Once the download has been completed the LCD should display "Saving..." then "Installing..." before rebooting.
1. On reboot the device will go through several stages of verification while upgrading the image downloaded from mBed Cloud. The firmware indicator should be blinking YELLOW during this process.
1. Upon a successful upgrade you should see the new version label you edited in the [previous step](#example-edit-version) on the LCD display next to "Version:" on the top line.
