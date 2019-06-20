
Description
-----------
A set of C++ libraries for a PC and a TI LaunchPad to communicating via serial line with [COBS encoding](https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing).

Setup
-----
This library has been tested with two TI TM4C LaunchPads (one `MainHSK` and one `MagnetHSK`) connected as follows:
1. Between the `MainHSK` board and the `MagnetHSK` board:
    * GND to GND
    * VBUS to VBUS
    * PB0 (main) to PB1 (magnet)
    * PB1 (main) to PB0 (magnet)
2. Between `MainHSK` and a PC
    * Long-side micro-usb, in software named 'Serial'

### Adding the Energia Library
1. Open a launchpad_src file in Energia
2. Go to `Sketch` -> `Include Library` -> `Add .ZIP Library`
3. Navigate to your download of the `launchpad_lib` folder, and select it.


Examples
--------
Please note that to run an example, you must include the example files (.h & .cpp) in the `main.cpp` file.

### PingAndRead
This example prompts the user for an intended destination and command, then sends out a COBS encoded data packet to that destination. It is recommended that the example be used with two TI TM4C boards. One should be flashed with `MainHSK_prototyp` in `launchpad_src`, and the other with `MagnetHSK_prototype` in the same folder.

If the boards are connected as described in Setup, commands will be passed downstream to their intended destination. The boards will be programmed with an analogous example, `CommandResponse`, in which their command functions are held.

Compiling
---------
Navigate to inside the src folder

### On Windows:
Compile command: `$ g++ -std=c++11 *.cpp -o testprogram`

Run command: `$ testprogram`

### On Linux:
Compile command: `$ g++ -std=c++11 *.cpp -o testprogram`

Run command: `$ sudo ./testprogram`


### Contributors

Richard Mueller & Keith McBride
