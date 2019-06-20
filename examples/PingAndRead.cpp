/* 
 * PingAndRead.cpp
 *
 * General template functions for a specific use case of the main.cpp program.
 * --Program prompts the user for an intended destination and command, then sends 
 * 	 out a COBS encoded data packet to that destination.
 *
 */

/*****************************************************************************
 * Defines
 ****************************************************************************/
#include "PingAndRead.h"

using std::cout;
using std::cin;
using std::endl;

/* Dummy pointer used in whatToDoIfFSR() and matchData() */
uint8_t * ptr;

/* User input variables for setupMyPacket() */
char userIN;
char userIN2;
housekeeping_id userDST;
housekeeping_cmd userCMD;

/* Buffer for outgoing data in setupMyPacket(). This can be changed if  */
uint8_t outgoingData [255] = {0};

/*****************************************************************************
 * Functions
 ****************************************************************************/
/* Function flow:
 * --Prints out interface protocol for addressing & commanding downstream devices
 * --Elaborates typedefs located in iProtocol.h
 *
 */
void startUp()
{
	cout << endl << "Destination typedefs:" << '\t' << '\t' << '\t' << '\t';
	cout << "Command typedefs:" << endl;
	cout << "SFC = 0" << endl;
	cout << "MainHSK = 1" << '\t' << '\t' << '\t' << '\t';
	cout << "	Ping Pong = 0" << endl;
	cout << "MagnetHSK = 2" << '\t' << '\t' << '\t' << '\t';	
	cout << "	FSR (fake sensor read) = 2" << endl <<endl;	
}

/* Function flow:
 * --Asks & accepts user input for intended destination & intended command to be 
 * 	 sent.
 * --Builds outgoing data header
 *
 * Function params:
 * hdr_out:		Pointer to the first byte of the outgoing packet
 * 				--As a housekeeping_hdr_t type, contains a src, dst, cmd, & len
 *
 * Function variables:
 * userIN:		Buffer for user inputted destination device (see typedefs)
 * userIN2:		Buffer for user inputted command (see typedefs)
 * userDST:		User inputted destination that is put in the outgoing packet
 * userCMD:		User inputted command that is put in the outgoing packet
 *
 */
void setupMyPacket(housekeeping_hdr_t * hdr_out)
{	
	/* Get user input for message destination */
	cout << "Destination #? " << endl;
	cin >> userIN;
	while (userIN)
	{
		if (userIN == '1') 
		{
			userDST = eMainHsk;
			break;
		}
		else if (userIN == '2')
		{
			userDST = eMagnetHsk;
			break;
		}
		else 
		{
			cout << "don't think so" << endl << endl;
			cout << "Destination #? " << endl;
			cin >> userIN;
		}
	}
	
	/* Get user input for message command */
	cout << "What command #? " << endl;
	cin >> userIN2;
	while (userIN2)
	{
		if (userIN2 == '0') 
		{
			userCMD = ePingPong;
			break;
		}
		else if (userIN2 == '2')
		{
			userCMD = eFakeSensorRead;
			break;
		}
		else 
		{
			cout << "don't think so" << endl;
			cout << "Command #? " << endl;
			cin >> userIN2;
		}
	}
	
	cout << endl;
	
	/* Fill in the header */
	hdr_out->dst = userDST;			// Intended destination of packet
	hdr_out->cmd = userCMD;			// Command for what do to with packet
	hdr_out->len = 0;				// Size of data after the header
	
	/* Option to match data, if wanted to populate outgoingData ahead of time */
	matchData(hdr_out);
}

/* Function flow:
 * --If a ping pong command is registered in the incoming packet, this function 
 * 	 reads off the contents of the header (src, dst, cmd, len)
 *
 * Function params:
 * hdr_in:		Pointer to the first byte of the incoming packet
 * 				--As a housekeeping_hdr_t type, contains a src, dst, cmd, & len
 *
 */
void whatToDoIfPingPong(housekeeping_hdr_t * hdr_in)
{
	cout << "Reading in packet header... " << endl;
	/* Read off header data */
	cout << "Packet source: " << (int) hdr_in->src << endl;
	cout << "Intended destination: " << (int) hdr_in->dst << endl;
	cout << "Command : " << (int) hdr_in->cmd << endl;
	cout << "Length of data attached: " << (int) hdr_in->len << endl << endl;
	cout << endl << endl;
}

/* Function flow:
 * --If a fake sensor read (FSR) command is registered in the incoming packet, 
 * 	 this function reads off the contents of the header (src, dst, cmd, len)
 * --It then reads off the data attached in the packet
 *
 * Function params:
 * hdr_in:		Pointer to the first byte of the incoming packet
 * 				--As a housekeeping_hdr_t type, contains a src, dst, cmd, & len
 *
 * Function variables:
 * ptr:			Dummy pointer to the current data location. Gets iterated over
 * 
 */
void whatToDoIfFSR(housekeeping_hdr_t * hdr_in)
{
	cout << "Reading in packet header... " << endl;
	/* Read off header data */
	cout << "Packet source: " << (int) hdr_in->src << endl;
	cout << "Intended destination: " << (int) hdr_in->dst << endl;
	cout << "Command : " << (int) hdr_in->cmd << endl;
	cout << "Length of data attached: " << (int) hdr_in->len << endl;
	cout << "Fake data: " << endl;
	
	ptr = (uint8_t *) hdr_in;
	
	for (int i = 0; i < hdr_in->len; i++)
	{
		cout << (int) *(ptr+4+i) << ", ";
	}
	cout << endl << endl;
}

/* Function flow:
 * --Matches an array of data with the (global) outgoingPacket data slots
 * --Requires outgoingData as a global uint8_tarray 
 *
 * Function params:
 * hdr_out:		Pointer to the first byte of the incoming packet
 * 				--As a housekeeping_hdr_t type, contains a src, dst, cmd, & len
 *
 * Function variables:
 * ptr:			Dummy pointer to where we want to put this byte in the outgoing
 *				packet. Gets iterated over
 * 
 */
void matchData(housekeeping_hdr_t * hdr_out)
{
	ptr = (uint8_t *) hdr_out;
	
	for (int i = 0; i < hdr_out->len; i++)
	{
		*(ptr+4+i) = outgoingData[i];
	}
}
