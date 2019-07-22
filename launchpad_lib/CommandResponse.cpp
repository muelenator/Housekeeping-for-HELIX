/*
 * CommandResponse.cpp
 * 
 * Defines a set of functions to act as responses to received commands and 
 * error protocols
 *
 */

/*******************************************************************************
* Defines
*******************************************************************************/
#include "CommandResponse.h"

/* Buffer for outgoing data */
uint8_t outgoingData [255] = {0};

/* Variables for performing a temperature reading + storing it in an array of uint8_t */
uint32_t TempRead;
uint32_t * tmp;

/*******************************************************************************
* Functions
*******************************************************************************/
/* Function flow:
 * --Fills outgoing header w/protocol standard
 * --Board response to ping pong command return
 * 
 * Function params:
 * hdr_out:		Pointer to outgoing packet header
 */
void whatToDoIfPingPong(housekeeping_hdr_t * hdr_out)
{
	/* Create the header */
	hdr_out->dst = eSFC;		// Intended destination of packet
	hdr_out->cmd = ePingPong;	// Command for what do to with packet
	hdr_out->len = 0;			// Size of data after the header
}

/* Function flow:
 * --Fills outgoing header w/protocol standard
 * --Takes a reading from its internal temp sensor
 * --Fills outgoing data buffer with the 32 bit reading
 * 
 * Function params:
 * hdr_out:		Pointer to outgoing packet header
 * */
void whatToDoIfISR(housekeeping_hdr_t * hdr_out)
{	
	/* Create the header */
	hdr_out->dst = eSFC;			// Intended destination of packet
	hdr_out->cmd = eIntSensorRead;	// Command for what do to with packet
	hdr_out->len = 4;				// Size of data after the header
	
	TempRead = analogRead(TEMPSENSOR);
	
	tmp = &TempRead;
	
	/* Fills outgoing data buffer */
	for(int i=0; i < hdr_out->len; i++)
    {
        outgoingData[i] = *tmp;
        *tmp = *tmp>>8;
    }
	
	matchData(hdr_out);
}

/* Function flow:
 * --Fills outgoing header w/protocol standard
 * --Fills outgoing data array with its internal device list
 * 
 * Function params:
 * hdr_out:		Pointer to outgoing packet header
 * downDevs:	The device's 2d array of downstream devices
 * num:			Total number of downstream devices
 *
 *  */
void whatToDoIfMap(housekeeping_hdr_t * hdr_out, uint8_t (&downDevs)[7][254], uint8_t num)
{
	/* Create the header */
	hdr_out->dst = eSFC;			// Intended destination of packet
	hdr_out->cmd = eMapDevices;		// Command for what do to with packet
	hdr_out->len = num;				// Size of data after the header
	
	if (num == 0) return;
	
	/* Fill in data array with device list */
	for (int i=0; i < 7; i++)
	{
		for (int j=0; j<254; j++)
		{
			if (downDevs[i][j] != 0)
			{
				outgoingData[--num] = j;
			}
		}
	}
	
	matchData(hdr_out);
}

/* Function flow:
 * --Finds the priority settings from the incoming packet
 * --Changes the intended command's priority
 * --Fills outgoing packet header w/protocol standard
 * --Sets outgoing data bytes to the new priority settings for SFC confirmation
 * 		--Not part of HSK protocol
 * 
 * Function params:
 * hdr_prio:	Pointer to the priority header attached to the incoming packet
 * hdr_out:		Pointer to outgoing packet header
 * comPriorList:	List of all command priorities
 * 
 *  */
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

/* Function flow:
 * --Defines two variables, val & channel, to store a channel's pot value
 * --Fills outgoing packet header w/protocol standard
 * --Writes to the heater control board (?) commands it can understand
 * --Waits for a reply byte to verify the message was received
 * --Sends that reply byte back to the SFC for verification
 * 
 * Function params:
 * hdr_in:		Incoming header pointer, can find the data pts from the location
 * hdr_out:		Pointer to the outgoing packet header
 * stream:		Serial port where the heater control is connected
 * 
 * Function variables:
 * val:			Potentiometer value to set
 * channel:		Which potentiometer
 * 
 *  */
void whatToDoIfHeaterControl(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out, Stream & stream)
{
	uint8_t val;
	uint8_t channel;
	
	hdr_out->dst = eSFC;
	hdr_out->cmd = eHeaterControl;
	hdr_out->len = 1;
	
	/* These two  */
	stream.write(254);
	stream.write(171);
	
	/* If 1 byte, change all channels */
    if (hdr_in->len == 1)
	{
        val= (uint8_t) *((uint8_t *) hdr_in + 4);
		stream.write(val);
    }
        
	else if(hdr_in->len == 2)
	{
		channel	 = 	(uint8_t) *((uint8_t *) hdr_in + 4 + 1);
        val		 = 	(uint8_t) *((uint8_t *) hdr_in + 4 + 2);
		
        for(int i=0; i < 4; i++)
		{
			stream.write(channel);
			stream.write(val);
        }
    }
	
	/* Wait for that confirmation byte */
	while (stream.read() < 0)
	{
	}

	/* Fill outgoing data with that byte */
	outgoingData[0] = (uint8_t) stream.read();
	matchData(hdr_out);
}

/* Function flow:
 * --Checks if this device has any commands of a certain priority
 * --Checks if the incoming destination is eBroadcast
 * --Overall, makes sure the device responds when it has no commands of a certain
 *   priority only if the destination is not eBroadcast
 * 
 * Function params:
 * hdr_in:		Pointer to the incoming packet header
 * hdr_out:		Pointer to the outgoing packet header
 * numSends:	Number of commands this device has of a certain priority
 *
 *  
 */
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

/* These functions are called if the device has detected an error.
 * They follow the HSK error protocol by filling the outgoing header &
 * outgoing error header.
 * 
 *  */
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

/* Function flow:
 * --Dereferences the outgoing packet data location and fills it with the data
 *   from a function which was executed prior
 * 
 * Function params:
 * hdr_out:		Pointer to the outgoing packet header
 * 
 */
void matchData(housekeeping_hdr_t * hdr_out)
{
	/* Each command response sets a data length */
	for (int i = 0; i < hdr_out->len; i++)
	{
		/* Dereference the outgoing packet's data location & fills the spot */
		*((uint8_t *)hdr_out+4+i) = outgoingData[i];
	}
}
