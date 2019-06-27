
Description
-----------
A set of C++ libraries for the HELIX Housekeeping board project. The project is split into two parts: libraries for the PC side of the operation (src) and libraries for daisy-chained (or otherwise connected) TI TM4C123GXL Launchpad devices.

This project utilizes COBS encoding for sending data packets. The usefulness of COBS encoding is that it signals the end of a data packet with a unique marker byte. This 'packet marker' is able to be unique (w.r.t. the other bytes in the stream) because COBS-encoding removes any instances of the byte within the packet itself. COBS can be advantageous because it creates small and predictable overhead. For more on COBS, see [COBS encoding](https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing). This software also implements libraries from [PacketSerial](https://github.com/bakercp/PacketSerial).


Setup
-----
This library has been tested with two TI TM4C LaunchPads (one acting as `MainHSK` and one acting as `MagnetHSK`).

### Connections
The boards are connected as follows:
1. Between the `MainHSK` board and the `MagnetHSK` board:
    * GND to GND
    * VBUS to VBUS
    * PB0 (main) to PB1 (magnet)
    * PB1 (main) to PB0 (magnet)
2. Between `MainHSK` and a PC
    * Micro-usb (main) to USB (PC)

### Setting up the Launchpad software
Install the [Energia IDE](http://energia.nu/download/) on your PC.
* Installing the board to the IDE
	1. Go to `Tools`->`Board`->`Boards Manager`
    2. Search for "TM4C"
    3. Install version `1.02` of the only library that pops up. Version `1.03` seems to be broken.
* If on Windows, install the Stellaris drivers.
	1. Download the Stellaris driver from [here](http://www.ti.com/tool/stellaris_icdi_drivers).
	2. On windows, go to `Device Manager` -> `Other Devices`
	3. Right click either device and hit Update Driver
	4. Navigate to the folder where you downloaded the Stellaris driver and click select
	5. Restart your computer. For more help, go to [here](https://www.14core.com/installing-launchpad-driver-with-energia-on-windows/).

#### Adding the Energia Library
1. Open a launchpad_src file in Energia
2. Go to `Sketch` -> `Include Library` -> `Add .ZIP Library`
3. Navigate to your download of the `launchpad_lib` folder, select it, and add it.

#### Flashing the Launchbad boards
Launchboard files are located in `launchpad_src`. Make sure one Launchpad is flashed with the `MainHsk_prototype.ino` file and is directly connected to the PC via USB, and the other is flashed with `MagnetHsk_prototype.ino`.

### Compiling with g++

In the `src`->`main.cpp` file, change the string `port_name` to whatever serial port your MainHSK Launchpad is using in your OS.

Navigate to inside the `src` folder. The compile command is the same for both Windows & Linux:    `$ g++ -std=c++11 *.cpp -o testprogram`.    To run,

#### On Windows:
Run command: `$ testprogram`

#### On Linux:
Run command: `$ sudo ./testprogram`

### Data structure
Each data packet is sent with an attached header before the start of the first data byte. The header contains:
1. The source the message came from
2. The intended final destination of the message
3. The command that the source wants to convey
4. The length of the attached data array after the header (after this length byte).

If a device needs to send a detected error to the SFC, an error header will be attached for a total of 8 bytes per error diagnostic. No data should be attached after an error header. The error header parameters are
1. The device the message originated from
2. The intended final destination of the message
3. The original command that message was sent with
4. The error type that was detected

If a set priority command is sent (see `Command: send priority setting`), two additional data bytes are required in addition to the packet header. Again, no data bytes should be attached past the priority header structure. The two parameters are
1. The command whose priority the user wishes to change
2. The priority level that the user wants that command to be characterized by


At the end of any data array (whether the packet includes actual data, an additional header structure, or nothing else), a checksum will be attached. Here, the checksum algorithm being implemented will simply subtract each byte of the packet, including the header, and take the modulus % 255. So, at minimum, there will be 5 bytes sent in a packet, not including COBS encoding bytes.

Testing
--------
The software acts as if the PC is the Science Flight Computer (SFC). On startup, the program

1. Prints a header with device #'s for addressing, command #'s, error types, and send priority #'s.
2. Asks the user for a destination to send a data packet to.
3. Asks the user for a command to send. If a command is not yet programmed (i.e. not in the program header) the devices should respond with an error (bad command) and the PC should reset all devices.
    * Note: The software reset has not been completely worked out yet. See `Error & restart testing` below for details.

### Implementation
Each of the Launchpad's serial ports are initialized from the get-go. However, they will not know which device is on the other end of a serial bus. For a Launchpad to obtain information about what device is on the other end, it must first receive a message from that serial line. This section is dedicated fundamental functions of the Housekeeping system as it is currently developed and the commands which are used to facilitate them.

#### Commands
* `ePingPong    0`: If this command is sent to a device from the SFC, that device will respond by sending the command back to the SFC in kind. Its own address will be in the source argument of the header. ePingPong helps discover which devices are active.
* `eSetPriority 1`: Changes the send priority of a certain command to one of four options: no priority, low priority, medium priority, or high priority. This command requires two additional data bytes to inputted/sent.
* `FakeSensorRead   2`: This command is meant to imitate a sensor read from a Launchpad. Once this command is received, a message will be sent back to the SFC with this same command and an array of 5 fake data points which are hardcoded into each Launchpad.
* `eFakeError1  3`: Once this command is received, the devices will immitate a BADDEST error and send the bad news to the SFC. More on this in `Error & restart testing` section 1b.
* `eFakeError2  4`: Similarly, this command will make a device imitate a BADARGS error and send this information to back to the SFC. More on this in `Error & restart testing` section 2b.
* `eMapDevices  5`: If a device receives this command, it will send the SFC the number of devices connected to its other serial ports along with their device addresses. More on this below in `Producing a device map`.
* `eSendLowPriority 250`: Instructs a device to executes all commands with the Low Priority characteristic.
    1. Note: if a device does not have any commands set with a specific priority, they will not respond if the initial destination was `eBroadcast`, but will respond if they are called specifically in the destination. See `Command: send priority setting`.
* `eSendMedPriority 251`: Instructs a device to executes all commands with the Medium Priority characteristic.
* `eSendHiPriority 252`: Instructs a device to executes all commands with the High Priority characteristic.
* `eSendAll 253`: Executes all available commands which produce data readings.
* `eReset 254`: Resets the device. Still in development.

#### Producing a device map
A main function of this software is to autonomously create a list of downstream devices. Note that devices are connected linearly through the serial bus: devices connected to the serial line opposite the direction of the SFC are considered downstream. From this list, the device software will be able to match an intended destination with the correct serial port, which will directly connect the SFC to any device.
* Note: this feature is waiting to be tested with more than 1 downstream serial port per device.

This device map can be produced by using the eBroadcast destination as follows:

1. After starting the program, at the prompt input `3` (eBroadcast) as the destination and `0` (ePingPong) as the command.
2. You should receive a message from both devices. As a way to check this, the PC will display the header of all messages it received before timeout (.25 seconds seemed reasonable).
3. By this time, all devices should have acknowledged the presence of any downstream devices. For a list of attached devices, input `3` (eBroadcast) as the destination and `5` (eMapDevices, a custom function for testing purposes) as the command.
4. You should see a message from each Launchpad containing the number of devices it has attached, and what their addresses #'s are (as shown in the program header).
    * Note: there is a bug when the Magnet HSK board is contacted for the first time. It responds with a random source address instead of its own (eMagnetHsk = 2). This initial flaw can be rectified by performing a software reset of the device, a process described in the next section.

#### Error & restart testing
Once an error is received, the software on the SFC (the PC) is programmed to display the error type and a log of all previous errors. After displaying the error information, an eReset command will be sent to all devices (eBroadcast destination).

* The reset feature is still in development. Currently, only the list of downstream devices and command priority list will be reset.

The error protocol programmed on the Launchpads can be tested following cases:
1. The device does not think that the destination address is downstream from it. This produces the error type BADDEST.
    * `Error test 1: Bad destination`. To test this error, 
        1. If you have not yet used the eBroadcast destination, input the intended destination `2` (eMagnetHsk) and command `0` (ePingPong). This should produce a BADDEST error (type -1) because the Main HSK does not yet know that a device is attached. The prescence of a downstream devcice is acknowledged once a packet is received from its serial line.
        2. This test can be used to verify that the BADDEST error function is working & being received. If you have already produced a device map, you can choose any of the three destinations and choose the command `3`. This should produce a BADDEST error message from the device(s) you chose.
2. The device receives a command that it does not know how to execute. This produces the error type BADARGS or BADCOMMAND, depending on the situation.
    * `Error test 2: Bad arguments`. To test this error,
        1. As stated previously, an error will pop up if you choose any command that isn't programmed into the launchpads. Choosing command #'s that aren't in the program header will produce a BADCOMMAND error. 
        2. This test can be used to verify that the BADARGS error function is working & being received. If you have already produced a device map, you can choose a known destination and choose the command `4`. This should produce a BADARGS error message from the device(s) you chose.

#### Command: send priority setting
It may sometimes be necessary to acquire groups of data readings from a device as opposed to single readings. For this task, we can set the 'priority' of a specific command/data reading to be low, medium, high, or none. This enables the user to ask a device to send all data of a given priority if needed. 
1. To set the priority of a command on a device, or on all devices, choose your destination # and input `1` for the command #. You will be prompted to provide two additional arguments. 
    * The first argument you will be prompted for is the command whose priority you wish to set. 
    * The second is the priority level you want to attach to that command.

If all goes well, you should receive a confirmation message that the device has changed its priority. Though, to be absolutely sure that a command's priority has been changed, you could ask the device to execute all commands of that priority.
* Choose the same destination(s) as above. For the command, compare the program header 'eSend' commands to the priority type you want. Input that number. The device(s) should execute the command whose priority you previously set, and any others that are at the same priority level.
    * For example, try changing eMapDevice's priority level to medium on the MainHSK & asking the device to send all medium priority level commands.

The Housekeeping system protocol is destination sensitive in the 'eSend' command category. A destination of a specific device address and the eBroadcast command will behave differently, even though they reach the same device with the same command. The following example shows the behavioral difference.
1. Pick a specific device you want to reach. Find a priority group that you have not assigned any commands to. Looking to the program header, use the 'eSend' command whose priority group matches the constraints in the last statement.
    * You should receive a confirmation message that the device did not have any commands set to this priority.
2. Now repeat the same steps, but this time choose the eBroadcast destination.
    * This time, the device we have been using in this example will not issue a confirmation. You might note that other devices will also respond if they happen to have a command set. 

#### Next steps
Fixing the board restart function. Adding support for more Launchpad serial ports. Also better code documentation and function & program flow descripitions.

Contributors
------------

Richard Mueller & Keith McBride
