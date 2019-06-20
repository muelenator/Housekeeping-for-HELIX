/* 
 * main.cpp
 *
 * Basics run of input/output for LaunchPad communication. All applicability is 
 * defined in myprogram.cpp. This file is not intended to be edited.
 *
 * --Serial port name on windows. If, in 'COM#', # is > 9, portName must include 
 * 	 these backslashes: '\\\\.\\COM#' 
 */
 
/*******************************************************************************
* Defines
*******************************************************************************/
#ifdef _WIN32
#include "win_src/SerialPort.h"
#include "win_src/SerialPort.cpp"
#endif

#ifdef __linux__
#include "linux_src/SerialPort_linux.h"
#include "linux_src/SerialPort_linux.cpp"
#include "linux_src/LinuxLib.h"
#include "linux_src/LinuxLib.cpp"
#endif 

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "iProtocol.h"
#include "../examples/PingAndRead.h"
#include "../examples/PingAndRead.cpp"

using std::cout;
using std::cin;
using std::endl;

/* Serial port name: see above */
const char *port_name = "COM6";

/* Name this device */
housekeeping_id myComputer = eSFC;

/* Declarations to keep track of device list */
uint8_t downStreamDevices[254] = {0};	// Keep a list of downstream devices
int numDevices = 0;			// Keep track of how many devices are downstream

/* Create buffers for data */
uint8_t outgoingPacket [MAX_PACKET_LENGTH] = {0};	// Buffer for outgoing packet
uint8_t incomingPacket [MAX_PACKET_LENGTH] = {0};	// Buffer for incoming packet

/* Defining the variable here makes the linker connect where these variables 
 * are being used (see iProtocol.h for declaration)	*/
housekeeping_hdr_t * hdr_in;
housekeeping_hdr_t * hdr_out;

/* Checksum value for read-in */
uint8_t checkin;

/*******************************************************************************
* Functions
*******************************************************************************/
/* Function flow:
 * --Labels the first 4 bytes of the incoming & outgoing data buffers as data headers
 * --Labels this computer's source for outgoing packets
 * --Computes the checksum of the outgoing data + attaches it to the end of the 
 * 	 message.
 *
 */
void setup()
{
	/* Point to data in a way that it can be read as a header_hdr_t */
	hdr_in = (housekeeping_hdr_t *) incomingPacket;	
	hdr_out = (housekeeping_hdr_t *) outgoingPacket;
	
	/* Create the header for the first message */
	hdr_out->src = myComputer;			// Source of data packet
	setupMyPacket(hdr_out);	// Fills in rest of header
	
	/* Reset number of found devices to 0 */
	numDevices = 0;
	
	/* Compute checksum for outgoing packet + add it to the end of packet */
	outgoingPacket[4+hdr_out->len] = computeMySum(outgoingPacket, 
											 outgoingPacket[4 + hdr_out->len]);
}

/* Function flow:
 * --Checks to see if the packet was received from an unknown device
 *		--If so, add that device address to the list of known devices
 * --Sorts through commands & executes defined functions in myprogram.h
 *
 * Function params:
 * buffer:		Pointer to the location of the incoming packet
 *
 * Function variables:
 * downStreamDevices:		The array of addresses of known devices
 * numDevices:				Running total of downstream devices
 * 
 */
void commandCentral(const uint8_t * buffer)
{
	/* Check if the device is already known */
	if (findMe(downStreamDevices, downStreamDevices+numDevices, hdr_in->src) 
			== downStreamDevices+numDevices)
	{
		/* If not, add it to the list of known devices */
		downStreamDevices[numDevices] = hdr_in->src;
		numDevices += 1;
	}
		
	/* Check commands */
	if (hdr_in->cmd == ePingPong)
	{
		whatToDoIfPingPong(hdr_in);
	}
	else if (hdr_in->cmd == eFakeSensorRead)
	{
		whatToDoIfFSR(hdr_in);
	}
}

/* Function flow:
 * --Called when a packet is received by the serial port instance
 * --Labels the first 4 bytes of the packet as data header constituents
 * --Checks to see if the packet is meant for this device
 *		--If so, check the checksum for data corruption + 
 *		  execute the command in the header
 *
 * Function params:
 * buffer:		Pointer to the location of the incoming packet
 * len:			Size of the decoded incoming packet
 * 
 */
void isItMe(const uint8_t * buffer, size_t len)
{
	/* Read in the header */
	hdr_in->src = buffer[0];
	hdr_in->dst = buffer[1];
	hdr_in->cmd = buffer[2];
	hdr_in->len = buffer[3];
	
	/* Check if the message was intended for this device
	 * If it was, check and execute the command  */
	if (hdr_in->dst == myComputer)
	{
		/* Check for data corruption */
		checkin = computeMySum(buffer, buffer[4 + hdr_in->len]);
		
		cout << "The computed checkSum is: " << (int) checkin << endl << endl;
		
		if (checkMySum(checkin, buffer[4 + hdr_in->len]))
		{
			commandCentral(buffer);
		}
		else cout << "Bummer, checksum did not match." << endl;
	}
	
	/* If it wasn't, we have problems */
	else
	{
		/* Throw an error */
		cout << "Error: wrong dst" << endl << endl;
		return;
	}
}


/*******************************************************************************
* Main program
*******************************************************************************/
int main()
{
    /* Declare an instance of the serial port connection */
 	SerialPort TM4C(port_name);
 	
  	/* Check if connection is established */
  	if (TM4C.isConnected()) cout << "Connection Established" << endl;
  	else 
	{
		cout << "ERROR, check port name";
		return 0;
	}
  	
	/* Set the function that will act when a packet is received */
	TM4C.setPacketHandler(&isItMe);
	
	/* Reset number of found devices to 0 */
	numDevices = 0;
	
	/* Start up your program & set the outgoing packet data */
	startUp();
	setup();
	
	/* send the header and packet*/
   	TM4C.send(outgoingPacket, 
			  sizeof(housekeeping_hdr_t)/sizeof(hdr_out->src) + hdr_out->len + 1); 
	
	/* While the serial port is open, */
  	while (TM4C.isConnected())
	{  	
		/* Reads in 1 byte at a time until the full packet arrives.
     	 * If a full packet is received, update will execute PacketReceivedFunction */	
    	int read_result = TM4C.update(incomingPacket);
    	if (read_result != 0)
    	{
    		setup();
    		TM4C.send(outgoingPacket, 
			 		  sizeof(housekeeping_hdr_t)/sizeof(hdr_out->src) + hdr_out->len + 1); 
		}
  	}
}
