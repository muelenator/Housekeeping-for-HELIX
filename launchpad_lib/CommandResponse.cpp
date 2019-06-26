#include "CommandResponse.h"

/* Buffer for outgoing data */
uint8_t outgoingData [255] = {0};


/* PC response to ping pong command return */
void whatToDoIfPingPong(housekeeping_hdr_t * hdr_out)
{
	/* Create the header */
	hdr_out->dst = eSFC;		// Intended destination of packet
	hdr_out->cmd = ePingPong;	// Command for what do to with packet
	hdr_out->len = 0;			// Size of data after the header
		
	matchData(hdr_out);
}

void whatToDoIfFSR(housekeeping_hdr_t * hdr_out)
{	
	/* Create the header */
	hdr_out->dst = eSFC;			// Intended destination of packet
	hdr_out->cmd = eFakeSensorRead;	// Command for what do to with packet
	hdr_out->len = 5;				// Size of data after the header
	
	/* Generate fake data array */
	outgoingData[0] = 1;
	outgoingData[1] = 1;
	for (int i = 2; i < hdr_out->len; i++) 
	{
		outgoingData[i] = outgoingData[i-1] + outgoingData[i-2];
	}
	
	matchData(hdr_out);
}

void error_baddest(housekeeping_hdr_t * hdr_in, housekeeping_err_t * hdr_err)
{
	hdr_err->src = (uint8_t) hdr_in->src;
	hdr_err->cmd = (uint8_t) hdr_in->cmd;
	hdr_err->error = (uint8_t) EBADDEST;
}

void error_badargs(housekeeping_hdr_t * hdr_in, housekeeping_err_t * hdr_err)
{
	hdr_err->src = (uint8_t) hdr_in->src;
	hdr_err->cmd = (uint8_t) hdr_in->cmd;
	hdr_err->error = (uint8_t) EBADARGS;
}

void whatToDoIfMap(housekeeping_hdr_t * hdr_out, uint8_t * downDevs, uint8_t num)
{
	/* Create the header */
	hdr_out->dst = eSFC;			// Intended destination of packet
	hdr_out->cmd = eMapDevices;		// Command for what do to with packet
	hdr_out->len = num;			// Size of data after the header
	
	/* Fill in data array with device list */
	for (int i=0; i < num; i++)
	{
		outgoingData[i] = *(downDevs+i);
	}
	
	matchData(hdr_out);
}

/* Match the outgoing packet w/the outgoing data */
void matchData(housekeeping_hdr_t * hdr_out)
{
	for (int i = 0; i < hdr_out->len; i++)
	{
		*((uint8_t *)hdr_out+4+i) = outgoingData[i];
	}
}
