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
1. Basic knowledge of executing commands from a shell or command prompt.
1. Python2 (NOTE: Python3 is not supported)

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
1. Grab the current version of the firmware using mbed-cli:
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
1. Once setup you can issue the make command to build the firmware.
   ```bash
   make
   ```
1. The resulting firmware image should be in BUILD/K64F/GCC_ARM/combined.bin
    * NOTE: The build directory may vary if building for a different target. The syntax for output directory is BUILD/\<target>/\<toolchain>

## Testing

## Example

