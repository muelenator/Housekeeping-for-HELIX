
Description
-----------
A set of C++ libraries for the HELIX Housekeeping system. The project is split into two parts: libraries for PC communication (SFC) and libraries for TI TM4C123GXL Launchpads. Communication between the two is through RS-232 serial.

This project currently utilizes COBS encoding for sending data packets. The usefulness of COBS encoding is that it signals the end of a data packet with a unique marker byte. This 'packet marker' is able to be unique (w.r.t. the other bytes in the stream) because COBS-encoding removes any instances of the byte within the packet itself. COBS can be advantageous because it creates small and predictable overhead while providing reliablity at high transfer speeds. For more on COBS, see [COBS encoding](https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing). 

This software also implements [PacketSerial](https://github.com/bakercp/PacketSerial), which wraps the HardwareSerial class for encoding/decoding COBS.

Additionally, a [variant of the Tiva-C library](https://github.com/jakeson21/Arduino-Tiva-Board-Energia-1.6.10) is used to gain access to the 'device' micro-usb connector on the Launchpad. The total number of available serial connections per board is 8.


Setup
-----
Install the [Energia IDE](http://energia.nu/download/) on your PC.

### Energia Software

#### Installing the Tiva-C library variant in the IDE
1. Go to `File`->`Preferences`->`Additional Board Manager URL's`
2. In the text box, paste this link: https://raw.githubusercontent.com/muelenator/Arduino-Tiva-Board-Energia-1.6.10/master/release/package_arduino-tiva-board-1.0.7_index.json,https://raw.githubusercontent.com/jakeson21/Arduino-Tiva-Board-Energia-1.6.10/master/release/package_arduino-tiva-board-1.0.7_index.json
3. Click `OK`, then navigate to `Tools`->`Board`->`Boards Manager`
4. At the bottom of the list, there should appear a board manager called `Arduino-Tiva Boards`. Install it.
* If on Windows, install the Stellaris drivers.
	1. Download the Stellaris driver from [here](http://www.ti.com/tool/stellaris_icdi_drivers).
	2. On windows, go to `Device Manager` -> `Other Devices`
	3. Right click either device and hit Update Driver
	4. Navigate to the folder where you downloaded the Stellaris driver and click select
	5. Restart your computer. For more help, go to [here](https://www.14core.com/installing-launchpad-driver-with-energia-on-windows/).
* (Optional) Installing the official TI Tiva-C board manager in the IDE
	1. Go to `Tools`->`Board`->`Boards Manager`
    2. Search for "TM4C"
    3. Install the latest version of the only library that pops up.

#### Adding the `launchpad_lib` library to the IDE
1. From `launchpad_src`, open a `.ino` file in Energia
2. Go to `Sketch` -> `Include Library` -> `Add .ZIP Library`
3. Navigate to your download of the `launchpad_lib` folder, select it, and add it.
4. Alternatively,
    * Copy the `launchpad_src` folder
    * Navigate to inside your Energia IDE library folder. On Windows, this is in `Documents`->`Energia`->`libraries`. On Linux, this is `Desktop`->`Energia`->`libraries`.
    * Paste the `launchpad_src` folder here.


#### Flashing the Launchpad boards
To gain access to the Device micro-usb port, the Launchpad must first be flashed with a compiled-binary sketch. This enables the board to change its bootloader. A binary flashing program like [UniFlash](http://www.ti.com/tool/uniflash) or, on Windows, [LM Flash Programmer](http://www.ti.com/tool/LMFLASHPROGRAMMER) will work.

Launchboard .ino files are located in `launchpad_src`. 
1. Load one into Energia and attach a Launchpad to the PC through the Launchpad's Debugging micro-usb port (with the switch in debugging mode). 
2. Under `Tools`->`Board: `, make sure that the `Arduino-Tiva 80 MHz` option is chosen. 
3. Export the compiled binary by `Sketch`->`Export compiled binary`. You should find the new .bin file in your sketch folder. 
* (If Uniflash) Select the board TM4C123GXL. Continue, and under `Flash image(s)` select the .bin file you just created and click `Load Image`. If all goes well, there should be a 'Program success' message in the terminal at the bottom.
* (If LM Flash) Select the board TM4C123G. Under `Program` select the .bin file you just created and click `Program`.

Future software flashing can take place through the Energia IDE by using the Launchpad's `Device` micro-usb port in conjunction with board manager `Arduino-Tiva 80MHz`.

### Connections

If the Launchpad source files were compiled & flashed with the "Arduino-Tiva" board manager, one Launchpad must be connected to a PC via USB through the Launchpad's `Device` micro-usb port. If the source files were compiled with the "TM4C123" board manager, the Launchpad's debugging port must be used. Using the TM4C123GXL Launchpad's pinout (found [here]()), connect `URX*` to `UTX*` (replace * with a port number) and vice versa. 

### Compiling with g++

In the `src`->`main.cpp` file, change the string `port_name` to whatever serial port your MainHSK Launchpad is using in your OS.

Navigate to inside the `src` folder. The compile command is the same for both Windows & Linux:    `$ g++ -std=c++11 *.cpp -o testprogram`.

**On Windows**: The run command is `$ testprogram`

**On Linux**: The run command is `$ sudo ./testprogram`. The sudo is there if the user does not have serial port access.

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

Protocol Testing
----------------
Each of the Launchpad's serial ports are initialized from the get-go. However, they will not know what devices are on each downstream serial port. For a Launchpad to obtain information about a device attached to a downstream port, it must first receive a message from that serial line. If a Launchpad does not know that a certain address is attached downstream, it will not pass on a message intended for that device. Each Launchpad is programmed to check the source address of upStream packets to verify if they are known. This way, the board creates a list of devices on each serial port. The program startup is intended to make sure each Launchpad is aware of every device attached downstream. This is accomplished through the `ePingPong` command. On program start, the PC sends out an `ePingPong` command to `eBroadcast`, so that every available device should respond back to the PC with the same command. 


### Sending a packet
The software acts as if the PC is the Science Flight Computer (SFC). On startup, the program prints a header with device #'s for addressing, command #'s, error types, and send priority #'s. It then prompts the user for:
1. A destination to send a data packet to.
2. A command to send. If a command is not yet programmed (i.e. not in the program header) the devices should respond with an error (bad command) and the PC should reset all devices and end the program.
3. A 'length' number of data bytes to attach in this packet. 
4. Two arguments for testing the system's protocol:
    1. Testing for a `Bad length`. The option is presented to send 'length' data bytes or none at all. Choosing a non-zero length and attaching zero bytes (typing `n`) in the packet should produce an error.
    2. Testing for a `Bad argument`. The checksum is computed & displayed. The user is given the option to enter the correct checksum or test the system by attaching an incorrect checksum. The latter should produce an error.

### Commands
* `ePingPong    0`: If this command is sent to a device from the SFC, that device will respond by sending the command back to the SFC in kind. Its own address will be in the source argument of the header. ePingPong helps discover which devices are active.
* `eSetPriority 1`: Changes the send priority of a certain command to one of four options: no priority, low priority, medium priority, or high priority. This command requires two additional arguments to inputted/sent.
* `(Test) IntSensorRead   2`: This command is meant to initiate an internal temperature read from a Launchpad. Once this command is received, a message will be sent back to the SFC with this same command and 4 data bytes which will be converted to a temperature (float) at the PC.
* `(Test) eMapDevices  3`: If a device receives this command, it will send the SFC the number of devices connected to its other serial ports along with their device addresses. More on this below in `Producing a device map`.
* `eSendLowPriority 250`: Instructs a device to executes all commands with the Low Priority characteristic.
    1. Note: if a device does not have any commands set with a specific priority, they will not respond if the initial destination was `eBroadcast`, but will respond if they are called specifically in the destination. See `Command: send priority setting`.
* `eSendMedPriority 251`: Instructs a device to executes all commands with the Medium Priority characteristic.
* `eSendHiPriority 252`: Instructs a device to executes all commands with the High Priority characteristic.
* `eSendAll 253`: Executes all available commands which produce data readings.
* `eReset 254`: Resets the device. Still in development.

### Producing a device map
A main function of this software is to autonomously create a list of downstream devices. Note that devices are connected linearly through the serial bus: devices connected to the serial line opposite the direction of the SFC are considered downstream. From this list, the device software will be able to match an intended destination with the correct serial port, which will directly connect the SFC to any device.

This device map can be produced by using the eBroadcast destination as follows:

1. After starting the program, at the prompt input
    * `255` (eBroadcast) as the destination
    * `0` (ePingPong) as the command
    * `0` as the length
    * `y/n` should not matter, since there are no bytes to include
    * `1`, the correct checksum
2. You should receive a message from all devices. As a way to check this, the PC will display the header of all messages it received before timeout (.25 seconds seemed reasonable).
3. By this time, all devices should have acknowledged the presence of any downstream devices. For a list of attached devices of each Launchpad, 
    * `3` (eBroadcast) as the destination
    * `5` (eMapDevices), a custom function for testing purposes, as the command.
    * `0` length, use the correct checksum
4. You should see a message from each Launchpad containing the number of devices it has attached and what their addresses #'s are (as shown in the program header).

### Error protocol testing

The Launchpad's programmed in accordance with this housekeeping protocol has errors grouped into four categories. They should throw an error in the following scenarios:

**Error: `Bad Destination -1`**
* A device receives a packet not intended for itself, and intended destination address is not in its device list. The packet will not continue to be forwarded downStream.
* TEST: In the destination prompt, use a destination address you know is not attached in your rig.

**Error: `Bad Command -2`**
* A device receives a packet intended for itself, but does not know the command in the packet header. Commands 2-249 are device specific.
* TEST: Ask a device programmed with MainHsk_prototype to execute a `eHeaterControl` command. This command is DCTHsk_prototype specific.

**Error: `Bad Length -3`**
* A device receives a packet intended for itself, but the data length section of the header does not match the number of data bytes received
* An incomplete packet is detected. The update function for a packet will expire after 1 second
* TEST: Use a command which does not send any additional data bytes from the PC. Choose a nonzero length and type `n` at the next prompt.

**Error: `Bad arguments -4`**
* A device receives a packet intended for itself, but the checksum calculated upon packet reception does not match the last byte received in the packet (the spot reserved for the checksum).
* TEST: When prompted for a checksum, use an incorrect one.



Once an error message is received from a Launchpad, the software on the PC is programmed to display the error type and a log of all errors since program start. After displaying the error information, the `eReset` command will be sent to all devices (`eBroadcast` destination) and the program will end. The device's address list will be cleared & all priority settings will be lost.

* The program must end at this point because USB CDC protocol will restart, sending unencoded data from the PC and the directly connected Launchpad.


### Command: send priority setting
To acquire groups of data readings from a device as opposed to single readings, we can set the 'priority' of a specific command/data reading to be low, medium, high, or none. This enables the user to ask a device to send all data of a given priority if needed. To set the priority of a command on a device, or on all devices,
* Choose your destination address(es)
* `1` as the command
* `2` as the length 
* `y` to send these bytes and press enter. The program will ask for these two bytes. They correspond to
    1. The first argument you will be prompted for is the command whose priority you wish to set. 
    2. The second is the priority level you want to attach to that command.
* Enter the correct checksum.

If all goes well, you should receive a confirmation message that the device has changed its priority. Though, to be absolutely sure that a command's priority has been changed, you could ask the device to execute all commands of that priority.
* Choose the same destination(s) as above.
* For the command, compare the program header 'eSend' commands to the priority type you want. Input that number. 
* 0 length, use the correct checksum

The device(s) should execute the command whose priority you previously set, and any others that are at the same priority level.
* For example, try changing eMapDevice's priority level to medium on the MainHSK & asking the device to send all medium priority level commands.

The Housekeeping system protocol is destination sensitive in the 'eSend' command category. A destination of a specific device address and the eBroadcast command will behave differently, even though they reach the same device with the same command. The following example shows the behavioral difference.
1. Pick a specific device you want to reach. Find a priority group that you have not assigned any commands to. Looking to the program header, use the 'eSend' command whose priority group matches the constraints in the last statement.
    * You should receive a confirmation message that the device did not have any commands set to this priority.
2. Now repeat the same steps, but this time choose the eBroadcast destination.
    * This time, the device we have been using in this example will not issue a confirmation. You might note that other devices will also respond if they happen to have a command set. 

### Adding a command
* Command enumerations are located in the `iProtocol.h` file of both the `launchpad_lib` and `src` folder. For both the PC and device to recognize the command, it must be added to both files.  
* For sorting through commands, there is a function called `commandCenter` in the `main.cpp` file (on the PC side) and in the `launchpad_src` source files (on the device side).
* Functions for command responses:
    * On the PC are located in the `userTest` source files in `src`
    * On the device side are located in the `commandResponse` source files in the `launchpad_lib` folder.

### Release notes
This build has been tested with a baud rate up to 1.5e7 bps. 
* The Linux side is pretty buggy. Setting a baud rate in Linux requires a prefix 'B' in front of the number, but only certain baud rates are enumerated and work this way. Because of some startup protocols I haven't been able to get rid of yet, you may have to exit the program once or twice before it works.


### Next steps
Linux reliability

Contributors
------------

Richard Mueller & Keith McBride
