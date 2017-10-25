## Software

### Requirements

To build the FOTA demonstration firmware, you need the following:

1. A computer that you can use to host the GCC toolchain and environment necessary to build the firmware.
    * Operating System and architectures supported:
      * Windows 7 or 10 (x86 or x86-64).
      * Linux (x86-64).
      * Mac OS X (x86-64).
    * A USB port and a type B micro USB cable to flash the K64F.
1. An internet connection to download the necessary software to build the firmware.
1. An Mbed Cloud 1.2 Account (does not work with previous versions of Mbed Cloud).
1. Basic knowledge of executing commands from a shell or command prompt.
1. The following utilties:
    * Python 2 (NOTE: This demonstration does not support Python 3).
    * Make, or equivalent utility, to execute a Makefile to build the target.
    * Bash or another shell equivalent.
    * cat.
    * awk.
    * grep.
    * sed.
    * git.
    * ln.
    * cp.
    * ctags.
    * rm.
    * manifest tool.
        * NOTE: You can install this with `pip install git+ssh://git@github.com/ARMmbed/manifest-tool-restricted.git@v1.2rc2`.
    * echo.
    * touch.

### Setup

1. <a id="setup-workspace"/>Create a workspace directory to host the toolchain environment and build the firmware.
1. <a id="setup-toolchain"/>Download the appropriate toolchain here:
    * [Windows](https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/6-2017q2/gcc-am-none-eabi-6-2017-q2-update-win32-sha2.exe).
    * [Linux](https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2).
    * [Mac OS X](https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-mac.tar.bz2).
1. <a id="setup-mbed-cli"/>Set up and install Mbed CLI using the [Mbed CLI setup](https://os.mbed.com/docs/v5.6/tools/setup.html) instructions.
1. <a id="setup-cloud-key"/>Obtain your Mbed Cloud key.
    1. Log in to the [Mbed Cloud Portal](https://portal.us-east-1.mbedcloud.com/login) with your username and password.
    1. Click on 'Manage access' and then 'API keys', and then select 'Create new API key'.
    1. Give a name to your API key and from the Group drop down box select 'Developers' and then hit create.
    1. On the next screen, you see the name you gave your key and the API key itself. Save the key to a file named ***mbed-cloud-key.txt***. You will use this later in the [instructions](#ins-cloud-key).
1. <a id="setup-cloud-cert"/>Obtain your Mbed Cloud certificate.
    1. Log in to the [Mbed Cloud Portal](https://portal.us-east-1.mbedcloud.com/login) with your username and password.
    1. Click on 'Device Identity' and then 'Certificates'; from the Actions drop down menu, select 'Create developer certificate'.
    1. Give your certificate a name and description (optional), and then click 'Create certificate'.
    1. If you successfully created the certificate, you are redirected to the 'Certificates' page. If you click on the name of the certificate in the window, a popup appears that has a button to download the certificate as a source file; click 'Download Developer C file'. You will use this later in the [instructions](#ins-cloud-cert).

### Instructions

1. Open a shell or command prompt, and change directories to the workspace you created in [setup](#setup-workspace).
1. <a id="ins-import-fota"/>Grab the current version of the firmware using Mbed CLI:

    ``` bash
    mbed import fota-demo
    ```
    
1. <a id="ins-cloud-key"/>Change directories to the `fota-demo` directory, and copy the `mbed-cloud-key.txt` file created in [setup](#setup-cloud-key) to ***.mbed-cloud-key***.
    * NOTE: There is a dot in front of the name and no extension on the filename.
1. <a id="ins-cloud-cert"/>Now, copy the ***mbed_cloud_dev_credentials.c*** source file that you downloaded in the [setup](#setup-cloud-cert) steps to `fota-demo` directory.
1. Next, set up the Mbed environment to find your toolchain by issuing the following command:

   ```bash
   mbed config -G GCC_ARM_PATH <workspace>/<gcc-toolchain-dir>/bin
   ```
   
    * NOTE: The path points to the `bin` directory where you extracted the GCC toolchain.
1. Using a text editor, open the ***mbed_app.json*** file, and change the `Wi-Fi SSID`, `Security Type` and `Password` fields to connect to a nearby AP with an internet connection.
    * NOTE: Ensure that the `value` field under the `wifi` key is set to true to enable Wi-Fi.
    * NOTE: If connecting to an OPEN network with no security, you do not need to set the password.
1. Once set up, you can issue the make command to build the firmware.

   ```bash
   make
   ```

1. <a id="ins-fw-loc"/>The resulting firmware image is in `BUILD/K64F/GCC_ARM/combined.bin`.
    * NOTE: The build directory may vary if building for a different target. The syntax for output directory is `BUILD/\<target>/\<toolchain>`.

### Testing

1. Follow the Mbed OS instructions for setting up the [PC Configuration](https://os.mbed.com/docs/v5.6/tutorials/windows-serial-driver.html).
1. Next, follow the instructions for [flashing a project binary](https://developer.mbed.org/platforms/FRDM-K64F/#flash-a-project-binary).
    * NOTE: Make sure the binary you copy to the board is the ***combined.bin*** image created in the [instructions](#ins-fw-loc) to the mounted drive.
1. Once you have copied the image, remove power from the device, and power it on again.
1. When the device is on, the power LED turns GREEN.
    1. When the device is connecting to the Wi-Fi station, it blinks YELLOW. Once it successfully connects, it turns solid BLUE. If it fails to connect, it turns solid RED.
    1. When the device is connecting to Mbed Cloud, the LED blinks YELLOW. Once it successfully connects to Mbed cloud, it turns solid BLUE. If it fails to connect, it turns solid RED.
    1. The top line of the LCD displays version information. The bottom line displays environmental data and network status.
    1. The colored LEDs illuminate data being sent to the cloud. The solid BLUE LEDs change to CYAN briefly and back to solid BLUE every time environmental data is sent to Mbed Cloud.

#### Typical start up

Using a serial console program, you can see the messages the device displays.

First, you see boot messages that start with "[BOOT]":

```
[BOOT] Active firmware integrity check:
[BOOT] [++++                           ]
[BOOT] [+++++++                                                               ]
```

After boot, messages from the app begin to display:

```
FOTA demo version: 1.0
     code version: 51e39c8-dev-dev-johnb01
init platform
sd init OK
init platform: OK
entering run loop
```

Mbed Cloud connection is successful when you see:

```
init mbed client
Client registered
Cloud Client: Ready
Internal Endpoint Name: 015dae3b5dbf000000000001001002a0
Endpoint Name: 015ce9867a9702420a011a0e03c00000
Device Id: 015dae3b5dbf000000000001001002a0
Account Id:
Security Mode (-1=not set, 0=psk, 1=<undef>, 2=cert, 3=none): 2
mbed client registered
```

### Example (FOTA)

1. Make sure your device is powered on and connected to mBed Cloud.
1. <a id="example-edit-version"/>In your workspace directory where the `fota-demo` directory was [checked out](#ins-import-fota) open up the ***mbed_app.json*** file and find the "version" key in the json file.
1. Change the "value" key underneath the version to a new number or string.
1. Open a shell and change directories to the `fota-demo` directory and execute the following command
    ```bash
    make && make campaign
    ```
    This command will rebuild your application. On a successful build it will upload the new image, then create a new manifest and campaign that will be sent to the cloud using the credentials stored in ***.mbed-cloud-key*** created in the [instructions](#setup-cloud-key)
1. Once the manifest and campaign have been uploaded you should see the LCD on the device display "Downloading..." and the firmware indicator should be blinking YELLOW.
    * Once the download has been completed the LCD should display "Saving..." then "Installing..." before rebooting.
1. On reboot the device will go through several stages of verification while upgrading the image downloaded from mBed Cloud. The firmware indicator should be blinking YELLOW during this process.
1. Upon a successful upgrade you should see the new version label you edited in the [previous step](#example-edit-version) on the LCD display next to "Version:" on the top line.

### Device states and indicators

This is an overview of some of the common states the device may be in.

| State           | Description   | Power LED | Wi-Fi LED | Cloud LED | Sensor LED | FOTA LED |
| --------------- | ------------- | --------- | -------- | --------- | ---------- | -------- |
| Power off       | Power switch in off position | Off | Off | Off | Off | Off |
| Wi-Fi connecting | Attempting to connect to Wi-Fi access point | Green | Flashing yellow | Off | Off | Off |
| Network scan    | Searching for Wi-Fi access points | Green | Flashing blue | Off | Off | Off |
| Wi-Fi connected  | Successful connection to Wi-Fi access point | Green | Blue | Off | Off | Off |
| Cloud connecting| Attempting to register with Mbed Cloud servers | Green | Blue | Flashing yellow | Blue | Off |
| Cloud connected | Successful registration with Mbed Cloud servers | Green | Blue | Blue | Blue | Off |
| FOTA download   | Received update compaign manifest, downloading firmware and rebooting | Green | Blue | Blue | Blue | Flashing yellow then blue |
| Firmware update | After reboot to install firmware | Green | Blue | Blue | Blue | Flashing yellow |
| Wi-Fi failed     | Lost connection or failed to connect to Wi-Fi access point | Green | Red | Off | Off | Off |
| Cloud failed    | Lost connection to Mbed Cloud | Green | Blue | Red | Blue | Off |
