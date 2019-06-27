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

void whatToDoIfMap(housekeeping_hdr_t * hdr_out, uint8_t * downDevs, uint8_t num)
{
	/* Create the header */
	hdr_out->dst = eSFC;			// Intended destination of packet
	hdr_out->cmd = eMapDevices;		// Command for what do to with packet
	hdr_out->len = num;				// Size of data after the header
	
	/* Fill in data array with device list */
	for (int i=0; i < num; i++)
	{
		outgoingData[i] = *(downDevs+i);
	}
	
	matchData(hdr_out);
}

void whatToDoIfSetPriority(housekeeping_prio_t * hdr_prio, housekeeping_hdr_t * hdr_out,
													    uint8_t * comPriorList)
{
	comPriorList[(uint8_t) hdr_prio->command] = (uint8_t) hdr_prio->prio_type;
	
	hdr_out->dst = eSFC;
	hdr_out->cmd = eSetPriority;
	hdr_out->len = 2;
	
	outgoingData[0] = hdr_prio->command;
	outgoingData[1] = hdr_prio->prio_type;
	
	matchData(hdr_out);
}

bool checkThatPriority(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out,
												    uint8_t numSends)
{
	if (hdr_in->dst != eBroadcast && numSends == 0)
	{
		hdr_out->dst = eSFC;
		hdr_out->cmd = hdr_in->cmd;
		hdr_out->len = 0;
		return true;
	}
	else return false;
}

void error_badDest(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out
											  , housekeeping_err_t * hdr_err)
{
	/* Outgoing housekeeping header */
	hdr_out->dst = eSFC;
	hdr_out->cmd = eError;
	hdr_out->len = 4;
	/* Outgoing error diagnostic */
	hdr_err->src = (uint8_t) hdr_in->src;
	hdr_err->cmd = (uint8_t) hdr_in->cmd;
	hdr_err->error = (uint8_t) EBADDEST;
}

void error_badLength(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out
											     , housekeeping_err_t * hdr_err)
{
	/* Outgoing housekeeping header */
	hdr_out->dst = eSFC;
	hdr_out->cmd = eError;
	hdr_out->len = 4;
	/* Outgoing error diagnostic */
	hdr_err->src = (uint8_t) hdr_in->src;
	hdr_err->cmd = (uint8_t) hdr_in->cmd;
	hdr_err->error = (uint8_t) EBADLEN;
}

void error_badCommand(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out
											     , housekeeping_err_t * hdr_err)
{
	/* Outgoing housekeeping header */
	hdr_out->dst = eSFC;
	hdr_out->cmd = eError;
	hdr_out->len = 4;
	/* Outgoing error diagnostic */
	hdr_err->src = (uint8_t) hdr_in->src;
	hdr_err->cmd = (uint8_t) hdr_in->cmd;
	hdr_err->error = (uint8_t) EBADCOMMAND;
}

void error_badArgs(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out
											  , housekeeping_err_t * hdr_err)
{
	/* Outgoing housekeeping header */
	hdr_out->dst = eSFC;
	hdr_out->cmd = eError;
	hdr_out->len = 4;
	/* Outgoing error diagnostic */
	hdr_err->src = (uint8_t) hdr_in->src;
	hdr_err->cmd = (uint8_t) hdr_in->cmd;
	hdr_err->error = (uint8_t) EBADARGS;
}

/* Match the outgoing packet w/the outgoing data */
void matchData(housekeeping_hdr_t * hdr_out)
{
	for (int i = 0; i < hdr_out->len; i++)
	{
		*((uint8_t *)hdr_out+4+i) = outgoingData[i];
	}
}
