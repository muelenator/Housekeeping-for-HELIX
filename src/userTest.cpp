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
#include "userTest.h"

using std::cout;
using std::cin;
using std::endl;

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
	cout << "#####################################################################";
	cout << endl;
	cout << "Destination typedefs:" << '\t' << '\t';
	cout << "Command typedefs:" << endl;
	
	cout << "SFC" << '\t' << '\t' << "0";
	cout << '\t' <<'\t';
	cout << "Ping Pong" << '\t' << '\t' << '\t' << "0" << endl;
	
	cout << "MainHSK" << '\t' << '\t' << "1";
	cout << '\t' << '\t';
	cout << "FSR (fake sensor read)" << '\t' << '\t' << "2" << endl;
	
	cout << "MagnetHSK" << '\t' << "2";
	cout << '\t' << '\t';	
	cout << "Bad dst Error Test" << '\t' << '\t' << "3" << endl;
	
	cout << "eBroadcast" << '\t' << "3";
	cout << '\t' << '\t';
	cout << "Bad args Error Test" << '\t' << '\t' << "4" << endl;
	
	cout << '\t' << '\t' << '\t' << '\t' << "Map all devices (dst=broadcast) 5";
	cout << endl << endl;

	cout << "Error codes:" <<  endl;
	cout << "EBADDEST" << '\t' << "-1" << endl;
	cout << "EBADCOMMAND" << '\t' << "-2" << endl;
	cout << "EBADLEN" << '\t' << '\t' << "-3" << endl;
	cout << "EBADARGS " << '\t' << "-4" << endl;
	cout << "#####################################################################";
	cout << endl << endl;
	
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
		else if (userIN == '3')
		{
			userDST = eBroadcast;
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
	cout << "What command #? (between 0 and 9) " << endl;
	cin >> userIN2;
	while (userIN2)
	{
		if ((int) userIN2 < 58 && (int) userIN2 > 47)
		{
			userCMD = (housekeeping_cmd) ((int) userIN2 - 48);
			break;
		}
		
		else 
		{
			cout << "don't think so" << endl;
			cout << "Command #? (between 0 and 10)" << endl;
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
	
	for (int i = 0; i < hdr_in->len; i++)
	{
		cout << (int) *((uint8_t *) hdr_in+4+i) << ", ";
	}
	cout << endl << endl;
}

void whatToDoIfError(housekeeping_err_t * hdr_err, uint8_t * errors, uint8_t & numError)
{
	/* Throw an error */
	cout << "Error received: Type " << (int) hdr_err->error -256 << endl << endl;
			
	/* Log the error */
	*(errors +4*numError) 	= hdr_err->src;
	*(errors +4*numError+1) = hdr_err->dst;
	*(errors +4*numError+2) = hdr_err->cmd;
	*(errors +4*numError+3) = hdr_err->error;
	numError+=1;
	
	/* Print out error history */
	cout << "ERROR LOG: Found " << (int) numError << " error(s)" << endl;

	for (int i=0; i<numError; i++)
	{
		cout << "Error #" << (i+1) << ": Origin device #";
		cout << (int) errors[4*i] << endl;
		cout << "Error #" << (i+1) << ": Intended for device #";
		cout << (int) errors[4*i+1] << endl;
		cout << "Error #" << (i+1) << ": With the attached command #";
		cout << (int) errors[4*i+2] << endl;
		cout << "Error #" << (i+1) << ": With error code ";
		cout << (int) errors[4*i+3]-256 << endl << endl;
	}
	
	cout << "Resetting downstream devices..." << endl << endl;
}

void whatToDoIfMap(housekeeping_hdr_t * hdr_in)
{
	cout << "Device #" << (int) hdr_in->src << " has attached devices:" << endl;
	
	for (int i=0; i < hdr_in->len; i++)
	{
		cout << (int) *((uint8_t *) hdr_in + 4 + i) << endl;
	}
	cout << endl;
}

void resetAll(housekeeping_hdr_t * hdr_out)
{
	hdr_out->dst = eBroadcast;
	hdr_out->cmd = eReset;
	hdr_out->len = 0;
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
	for (int i = 0; i < hdr_out->len; i++)
	{
		*((uint8_t *) hdr_out+4+i) = outgoingData[i];
	}
}
