/*
 * main.cpp
 *
 * Basic run of input/output for LaunchPad communication. All applicability is
 * defined in userTest.cpp. Only the USB port address is intended to be editted
 * in this file.
 *
 * --Serial port name on Windows. If, in 'COM#', # is > 9, portName must include
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
#include <chrono>
#include <cerrno>

#include "iProtocol.h"
#include "userTest.h"

using std::cout;
using std::cin;
using std::endl;

/* Serial port name: see above */
const char *port_name = "/dev/ttyACM0";

/* Name this device */
housekeeping_id myComputer = eSFC;

/* Declarations to keep track of device list */
uint8_t downStreamDevices[254] = {0};	// Keep a list of downstream devices
uint8_t numDevices = 0;			// Keep track of how many devices are downstream

/* Keep a log of errors */
uint8_t errorsReceived[254] = {0};
uint8_t numErrors = 0;

/* Create buffers for data */
uint8_t outgoingPacket [MAX_PACKET_LENGTH] = {0};	// Buffer for outgoing packet
uint8_t incomingPacket [MAX_PACKET_LENGTH] = {0};	// Buffer for incoming packet

/* Defining the variable here makes the linker connect where these variables
 * are being used (see iProtocol.h for declaration)	*/
housekeeping_hdr_t * hdr_in;
housekeeping_hdr_t * hdr_out;
housekeeping_err_t * hdr_err;
housekeeping_prio_t * hdr_prio;

/* Utility variableas: */
uint8_t checkin;				// Checksum value for read-in
uint8_t lengthBeingSent;		// Actual

uint16_t numberIN;
std::string numbufIN;

/* While connected, variables to check the how long its been since read a byte */
int read_result = 0;
std::chrono::time_point<std::chrono::system_clock> newest_zero, newest_result;
std::chrono::duration<double> elapsed_time;

/* Bool if a reset needs to happen */
bool needs_reset = false;

/*******************************************************************************
* Functions
*******************************************************************************/
/* Function flow:
 * --Gets outgoing header from the userTest function 'setupMyPacket'
 * --Computes the checksum of the outgoing data
 * --Asks user what checksum to use
 *
 * Function variables:
 * numbufIN:		Character buffer for user input
 * numberIN:		Unsigned integer that resulted from the conversion
 *
 */
void setup()
{
	lengthBeingSent = setupMyPacket(hdr_out, hdr_prio);	// Fills in rest of header

	/* Compute checksum for outgoing packet + add it to the end of packet */
	outgoingPacket[4+lengthBeingSent] = computeMySum(outgoingPacket,
											 &outgoingPacket[4 + lengthBeingSent]);

	cout << "The computed checksum is " << (int) outgoingPacket[4+lengthBeingSent];
	cout << ". Enter this value, or input a different checksum to check protocol reliability.";
	cout << endl;

	cin >> numbufIN;

	while (cin)
	{
		numberIN = strtoul(numbufIN.c_str(), 0, 10);

		if (numberIN > 255)
		{
			cout << "Number too big. Input a number between 0 & 255: " ;
			cin >> numbufIN;
		}
		else
		{
			outgoingPacket[4+lengthBeingSent] = (uint8_t) numberIN;
			break;
		}
	}
	cout << endl;
}

/* Function flow:
 * --Checks to see if the packet was received from an unknown device
 *		--If so, add that device address to the list of known devices
 * --Sorts through commands & executes defined functions in myprogram.h
 * --If there is no protocol for a command, just reads header + displays data
 *
 * Function params:
 * buffer:		Pointer to the location of the incoming packet
 *
 * Function variables:
 * downStreamDevices:		The array of addresses of known devices
 * numDevices:				Running total of downstream devices
 *
 */
void commandCenter(const uint8_t * buffer)
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
		justReadHeader(hdr_in);
		cout << endl;
	}
	else if (hdr_in->cmd == eSetPriority)
	{
		whatToDoIfSetPriority(hdr_in, hdr_prio);
	}
	else if (hdr_in->cmd == eIntSensorRead)
	{
		whatToDoIfISR(hdr_in);
	}
	else if (hdr_in->cmd == eMapDevices)
	{
		whatToDoIfMap(hdr_in);
	}
	else if ((int) hdr_in->cmd < eSendAll && (int) hdr_in->cmd >= eSendLowPriority
			 && hdr_in->len == 0)
	{
		cout << "Device #" << (int) hdr_in->src;
		cout << " did not have any data of this priority." << endl << endl;
	}
	else if (hdr_in->cmd == eError)
	{
		whatToDoIfError(hdr_err, errorsReceived, numErrors);

		resetAll(hdr_out);

		/* Compute checksum for outgoing packet + add it to the end of packet */
		outgoingPacket[4+hdr_out->len] = computeMySum(outgoingPacket,
											 &outgoingPacket[4 + hdr_out->len]);

		needs_reset = true;
	}
	else
	{
		justReadHeader(hdr_in);
		cout << "DATA: ";

		for (int i=0; i < hdr_in->len; i++)
		{
			cout << (int) *((uint8_t *) hdr_in + 4 + i) << " ";
		}
		cout << endl;
	}
}

/* Function flow:
 * --Called when a packet is received by the serial port instance
 * --Checks to see if the packet is meant for this device
 *		--If so, check the checksum for data corruption +
 *		  execute the command in the header
 *
 * Function params:
 * buffer:		Pointer to the location of the incoming packet
 * len:			Size of the decoded incoming packet
 *
 * Function variables:
 * checkin:		Utility variable to store a checksum in
 *
 */
void checkHdr(const uint8_t * buffer, size_t len)
{
	/* Check if the message was intended for this device
	 * If it was, check and execute the command  */
	if (hdr_in->dst == myComputer)
	{
		/* Check for data corruption */
		checkin = computeMySum(buffer, &buffer[4 + hdr_in->len]);

		cout << "The computed checkSum is: " << (int) checkin << endl << endl;

		if (checkMySum(checkin, buffer[4 + hdr_in->len]))
		{
			commandCenter(buffer);
		}
		else cout << "Bummer, checksum did not match." << endl;
	}

	/* If it wasn't, cast it as an error & restart */
	else
	{
		cout << "Bad destination received... Restarting downstream devices." << endl;

		resetAll(hdr_out);

		/* Compute checksum for outgoing packet + add it to the end of packet */
		outgoingPacket[4+hdr_out->len] = computeMySum(outgoingPacket,
											 &outgoingPacket[4 + hdr_out->len]);

		needs_reset = true;
	}
}


/*******************************************************************************
* Main program
*******************************************************************************/
int main()
{
	/* Point to data in a way that it can be read as known data structures */
	hdr_in = (housekeeping_hdr_t *) incomingPacket;
	hdr_out = (housekeeping_hdr_t *) outgoingPacket;
	hdr_err = (housekeeping_err_t *) (incomingPacket+4);
	hdr_prio = (housekeeping_prio_t *) (incomingPacket+4);

	/* Create the header for the first message */
	hdr_out->src = myComputer;			// Source of data packet

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
	TM4C.setPacketHandler(&checkHdr);

	/* Start up your program & set the outgoing packet data + send it out */
	startUp(hdr_out);
	outgoingPacket[4+hdr_out->len] = computeMySum(outgoingPacket,
											 &outgoingPacket[4 + hdr_out->len]);
	TM4C.send(outgoingPacket, 4 + hdr_out->len + 1);

	/* On startup: Reset number of found devices & errors to 0 */
	memset(downStreamDevices, 0, numDevices);
	numDevices = 0;
	memset(errorsReceived, 0, numErrors);
	numErrors = 0;

	/* Initialize timing variables for when the last message was received */
	newest_zero = std::chrono::system_clock::now();
	newest_result = std::chrono::system_clock::now();

	/* While the serial port is open, */
  while (TM4C.isConnected())
	{
		/* Reads in 1 byte at a time until the full packet arrives.
     	 * If a full packet is received, update will execute PacketReceivedFunction
		 * If no full packet is received, bytes are discarded 	*/
		read_result = TM4C.update(incomingPacket);

		/* If a packet was decoded, mark down the time it happened */
		if (read_result > 0) newest_result = std::chrono::system_clock::now();

		/* If a packet wasn't decoded in this run, write down the current time */
		else newest_zero = std::chrono::system_clock::now();

		/* Using the above variable definitions, calculate how long its been since
		 * something has been decoded */
		elapsed_time = newest_zero - newest_result;

		/* If that ^ time is greater than 1/2 a second, prompt the user again */
		if (elapsed_time.count() > .5)
		{
			/* Check if a reset needs to be sent */
			if (needs_reset)
			{
				TM4C.send(outgoingPacket, 4 + hdr_out->len + 1);
				needs_reset = false;
				return 0;
			}

			/* If it doesn't, prompt the user again for packet params  */
			setup();

			/* Send out the header and packet*/
			TM4C.send(outgoingPacket, 4 + lengthBeingSent + 1);

			/* Reset the timing system */
			newest_zero = std::chrono::system_clock::now();
			newest_result = std::chrono::system_clock::now();
		}
  }
}
