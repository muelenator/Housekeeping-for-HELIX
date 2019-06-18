#include "myprogram.h"
#include <iostream>

using std::cout;
using std::cin;
using std::endl;

/* User input variables declared */
char userIN;
char userIN2;
uint8_t * ptr;
housekeeping_id userDST;
housekeeping_cmd userCMD;

/* Buffer for outgoing data */
uint8_t outgoingData [255] = {0};

/* Startup function for user interface */
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

/* Setting the interface protocol for this device */
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
		else if (userIN = '2')
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
	
	/* Match the outgoing packet w/the outgoing data */
//	for (int i = 0; i < hdr_out->len; i++)
//	{
//		*(hdr_out->len + i) = outgoingData[i];
//	}
}

/* PC response to ping pong command return */
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
