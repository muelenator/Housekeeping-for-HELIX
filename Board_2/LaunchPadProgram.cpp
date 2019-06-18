#include "LaunchPadProgram.h"

/* Buffer for outgoing data */
uint8_t outgoingData [255] = {0};


/* PC response to ping pong command return */
void whatToDoIfPingPong(housekeeping_hdr_t * hdr_out)
{
	/* Create the header */
	hdr_out->dst = eSFC;	// Intended destination of packet
	hdr_out->cmd = ePingPong;	// Command for what do to with packet
	hdr_out->len = 0;// Size of data after the header
		
	matchData(hdr_out);
}

void whatToDoIfFSR(housekeeping_hdr_t * hdr_out)
{	
	/* Create the header */
	hdr_out->dst = eSFC;			// Intended destination of packet
	hdr_out->cmd = eFakeSensorRead;	// Command for what do to with packet
	hdr_out->len = 5;			// Size of data after the header
	
	/* Generate fake data array */
	outgoingData[0] = 1;
	for (int i = 1; i < 5; i++) 
	{
		outgoingData[i] = outgoingData[i-1] + outgoingData[i-2];
	}
	
	matchData(hdr_out);
}

/* Match the outgoing packet w/the outgoing data */
void matchData(housekeeping_hdr_t * hdr_out)
{
	uint8_t * ptr = (uint8_t *) hdr_out;
	
	for (int i = 0; i < hdr_out->len; i++)
	{
		*(ptr+4+i) = outgoingData[i];
	}
}
