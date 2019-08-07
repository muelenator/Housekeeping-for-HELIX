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
void whatToDoIfPingPong(uint8_t * data)
{
	*data = 0;
}

/* Function flow:
 * --Fills outgoing header w/protocol standard
 * --Takes a reading from its internal temp sensor
 * --Fills outgoing data buffer with the 32 bit reading
 * 
 * Function params:
 * hdr_out:		Pointer to outgoing packet header
 * */
void whatToDoIfISR(uint8_t * data)
{		
	TempRead = analogRead(TEMPSENSOR);
	
	tmp = &TempRead;
	
	/* Fills outgoing data buffer */
	for(int i=0; i < 4; i++)
    {
        *(data+i) = *tmp;
        *tmp = *tmp>>8;
    }
	
//	matchData(data);
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
int whatToDoIfHeaterControl(uint8_t * data, uint8_t len, uint8_t * respData)
{	
	/* If 1 byte, change all channels  to data byte*/ 
	// will write the serial.write to the linduino here
//	serial.write(254); // always write this first byte
	//needs to be about respData packets..
	int retval = 0;
	(0x01 & len) ? retval=1 : retval=2;
	*respData = 254;
	if (retval == 1) {
		*(respData + 1) = 170;
		*(respData + 2) = *data;
	}
	else if (retval == 2) {
		*(respData + 1) = 171;
		*(respData + 2) = *data;
		*(respData + 3) = *(data + 1);
	}
	else retval = EBADLEN;
//	(0x01 & len) ? serial.write(170) : serial.write(171);
	return retval;
	// now just write the next value, and if len >1 write the second byte in data also. 
//	stream.write(*data);
//	if ((int)len == 2) serial.write(*data + 1);
   	/* Fill outgoing data with that byte */
    // where do we put the data?
}

int whatToDoIfTestHeaterControl(uint8_t* data, uint8_t len)
{
	/* If 1 byte, change all channels  to data byte*/
	// will write the serial.write to the linduino here
//	serial.write(254); // always write this first byte
	//needs to be about respData packets..
	int retval = 0;
	(0x01 & len) ? retval = 1 : retval = 2;
	//	(0x01 & len) ? serial.write(170) : serial.write(171);
	return retval;
	// now just write the next value, and if len >1 write the second byte in data also. 
//	stream.write(*data);
//	if ((int)len == 2) serial.write(*data + 1);
	/* Fill outgoing data with that byte */
	// where do we put the data?
}
/*
void whatToDoIfTestMode(uint8_t* incomingdata)
{
//	uint16_t tmpval = *numTestPackets_p;
	uint8_t first = *incomingdata;
	uint8_t second = *(incomingdata + 1);
	// Fill outgoing data with the numTestPackets
    *data = first;
	*(data + 1) = second;
//	matchData(hdr_out);
	*numTestPackets_p--;
}
*/
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
/*
void matchData(housekeeping_hdr_t * hdr_out)
{
	// Each command response sets a data length 
	for (int i = 0; i < hdr_out->len; i++)
	{
		// Dereference the outgoing packet's data location & fills the spot
		*((uint8_t *)hdr_out+4+i) = outgoingData[i];
	}
}
*/