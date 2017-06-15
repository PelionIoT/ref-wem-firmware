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
	mbed import https://github.com/ARMmbed/fota-demo
	cd fota-demo
	```

fota-demo is now under ``~/workspace/fota-demo``.  You can look at ``main.cpp`` to familiarize yourself with the code.

## Compiling

The fota-demo project uses a Makefile to compile the source code.  The Makefile attempts to detect the toolchain and target and calls the mbed compiler with appropriate options.

```
make
```

Alternatively, the mbed compiler can be invoked manually.  Invoke `mbed compile`, specifying:

* Your board: ``-m <board_name>``
* Your toolchain: ``-t <`GCC_ARM`, `ARM` or `IAR`>``
* The gordon build profile: build_profile.json

For example, for the board K64F and GCC compiler:

```
mbed compile -t GCC_ARM -m K64F --profile build_profile.json
```

Your PC may take a few minutes to compile the code.  At the end you should see the following result:

```
+--------------------------+-------+-------+-------+
| Module                   | .text | .data |  .bss |
+--------------------------+-------+-------+-------+
| Fill                     |   122 |     0 |  2264 |
| Misc                     | 22593 |  2480 |   117 |
| features/storage         |    42 |     0 |   184 |
| hal                      |   418 |     0 |     8 |
| platform                 |   818 |     4 |   269 |
| rtos                     |    40 |     4 |     4 |
| rtos/rtx                 |  5841 |    20 |  6870 |
| targets/TARGET_Freescale |  4962 |    12 |   384 |
| Subtotals                | 34836 |  2520 | 10100 |
+--------------------------+-------+-------+-------+
Allocated Heap: 24580 bytes
Allocated Stack: unknown
Total Static RAM memory (data + bss): 12620 bytes
Total RAM memory (data + bss + heap + stack): 37200 bytes
Total Flash memory (text + data + misc): 38396 bytes

Image: ./BUILD/K64F/GCC_ARM/fota-demo.bin
```

The program file, ``fota-demo.bin``, is under your ``./BUILD/K64F/GCC_ARM/`` folder.

## Flashing your board

mbed Enabled boards are programmable by drag and drop over a USB connection.

1. Connect your mbed board to the computer over USB.
2. Copy the binary file to the board. In the example above, the file is ``fota-demo.bin``, and it's under the ``./BUILD/K64F/GCC_ARM/`` folder.
3. Press the reset button to start the program.

For more information, see the [DAPLINK](https://developer.mbed.org/handbook/DAPLink) documentation.
