/* 
 * ePP_v1.cpp
 *
 * Program writes its protocol header to the serial bus
 * with a ePingPong command and displays a response header.
 *
 */
 
/* Take out some of these libraries maybe eventually */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "SerialPort.h"
#include "iProtocol.h"
#include "myprogram.h"
#include "LinuxLib.h"

/*******************************************************************************
* Defines
--Serial port name on windows. If, in 'COM#', # is > 9, portName must include 
  these backslashes: '\\\\.\\COM#' 
*******************************************************************************/
using std::cout;
using std::cin;
using std::endl;

/* Serial port name on windows: see above */
const char *port_name = "/dev/ttyACM0";

/* Name this device */
housekeeping_id myComputer = eSFC;

/* Declarations to keep track of device list */
uint8_t downStreamDevices[254] = {0};	// Keep a list of downstream devices
int numDevices = 0;			// Keep track of how many devices are downstream

/* Create buffers for data */
uint8_t outgoingPacket [MAX_DATA_LENGTH] = {0};	// Buffer for outgoing packet
uint8_t incomingPacket [MAX_DATA_LENGTH] = {0};	// Buffer for incoming packet

/* Guess: defining the variable here makes the linker connect where these
 * 		  variables are being used 	*/
housekeeping_hdr_t * hdr_in;
housekeeping_hdr_t * hdr_out;

/* Checksum value for read-in */
uint8_t checkin;

/*******************************************************************************
* Functions
*******************************************************************************/
/* Setting the interface protocol for this device */
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

/* Sorts through commands */
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

/* Called when a packet is received by the microcontroller instance*/
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
	/*********************************************************
	* Opening the serial connection
	**********************************************************/
	/* Start up your program & set the outgoing packet data */
	// startUp();
	setup();
	
	/* Declare an instance of the serial port connection */
 	SerialPort TM4C(port_name);
 	
  	/* Check if connection is established */
  	if (TM4C.isConnected()) cout << "Connection Established" << endl;
  	else 
	{
		cout << "ERROR, check port name" << endl;
		return 0;
	}
  	
	/* Set the function that will act when a packet is received */
	TM4C.setPacketHandler(&isItMe);
	/* send the header and packet*/
   	TM4C.send(outgoingPacket, 
			  sizeof(hdr_out)/sizeof(hdr_out->src) + hdr_out->len + 1); 
	
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
			 		  sizeof(hdr_out)/sizeof(hdr_out->src) + hdr_out->len + 1); 
		}
  	}
}
