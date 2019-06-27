#include <PacketSerial.h>
#include <iProtocol.h>
#include <CommandResponse.h>

/* Declare an instance of PacketSerial to set up the serial bus */
PacketSerial upStream1;
PacketSerial downStream1;

/*******************************************************************************
* Defines
*******************************************************************************/
/* Name of this device */
housekeeping_id myID = eMagnetHsk;

/* Create buffers for incoming/outgoing packets */
uint8_t outgoingPacket [MAX_PACKET_LENGTH]; 	// Buffer for outgoing packet
uint8_t incomingPacket [MAX_PACKET_LENGTH];		// Buffer for incoming packet

/* Memory buffers for housekeeping system functions */
uint8_t downStreamDevices[254] = {0};	// Keep a list of downstream devices
uint8_t numDevices = 0;					// Keep track of how many devices are downstream
uint8_t commandPriority[255] = {0};		// Each command's priority takes up one byte

/* Utility variables for internal use */
uint8_t checkin;		// Used for comparing checksum values
size_t hdr_size = sizeof(housekeeping_hdr_t)/sizeof(hdr_out->src); // size of the header
uint8_t numSends = 0;	// Used to keep track of number of priority commands executed

/* Defining the variable here makes the linker connect where these variables 
 * are being used (see iProtocol.h for declaration)	*/
housekeeping_hdr_t * hdr_in;
housekeeping_hdr_t * hdr_out;
housekeeping_err_t * hdr_err;
housekeeping_prio_t * hdr_prio;

/*******************************************************************************
* Main program
*******************************************************************************/
void setup()
{
	Serial.begin(9600);
	upStream1.setStream(&Serial1);
	upStream1.setPacketHandler(&checkHdr);
  
	Serial1.begin(9600);
	downStream1.setStream(&Serial);
	downStream1.setPacketHandler(&checkHdr);
	
	/* Point to data in a way that it can be read as a header_hdr_t */
	hdr_in = (housekeeping_hdr_t *) incomingPacket;	
	hdr_out = (housekeeping_hdr_t *) outgoingPacket;
	hdr_err = (housekeeping_err_t *) (outgoingPacket + hdr_size);
	hdr_prio = (housekeeping_prio_t *) (incomingPacket + hdr_size);
}

void loop()
{
	/* Continuously read in one byte at a time until a packet is received */
	upStream1.update(incomingPacket);
	downStream1.update(incomingPacket);
}

/*******************************************************************************
* Functions
*******************************************************************************/
/* To be executed when a packet is received */
void checkHdr(const void * sender, const uint8_t * buffer, size_t len)
{	
	/* Default header & error data values */
	hdr_out->src = myID;	// Source of data packet
	
	if (hdr_in->dst == eBroadcast) hdr_err->dst = myID;
	else hdr_err->dst = hdr_in->dst;	// If an error occurs at this device from a message
	
	/* Check if the message is for this device. If so, check & execute command */
	if (hdr_in->dst == myID || hdr_in->dst == eBroadcast)
	{
		/* Check for data corruption */
		checkin = computeMySum(buffer, &buffer[4 + hdr_in->len]);
		
		if (checkMySum(checkin, buffer[4 + hdr_in->len]))
		{
			/* Forward downstream if eBroadcast */
			if (hdr_in->dst == eBroadcast)
			{
				downStream1.send(buffer, len);
			}
			
			/* If a send all priority command is received */
			if ((int) hdr_in->cmd < 253 && (int) hdr_in->cmd >= 250)
			{
				numSends = 0;
				/* Execute all commands of that type */
				for (int i = 0; i < 255; i++)
				{
					if (commandPriority[i] == (int) hdr_in->cmd -249)
					{
						hdr_in->cmd = (uint8_t) i;
						commandCenter();
						numSends++;
					}
				}
				/* If there are no commands of that type & the destination isn't eBroadcast*/
				if (checkThatPriority(hdr_in, hdr_out, numSends))
				{
					outgoingPacket[hdr_size + hdr_out->len] = computeMySum(outgoingPacket, 
																&outgoingPacket[hdr_size+hdr_out->len]);
					upStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
				}
			}
			/* If a send all command is received */
			else if (hdr_in->cmd == eSendAll)
			{
				for (int i = 2; i < 6; i++)
				{
					hdr_in->cmd = (uint8_t) i;
					commandCenter();
				}
			}
			/* Otherwise just execute the command */
			else commandCenter();
		}
		
		/* If the checksum didn't match, throw a bad args error */
		else
		{
			error_badArgs(hdr_in, hdr_out, hdr_err);	
			outgoingPacket[hdr_size + hdr_out->len] = computeMySum(outgoingPacket, 
															       &outgoingPacket[hdr_size+hdr_out->len]);
			upStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
		}
	}
  
	/* If the message wasn't meant for this device pass it along */
	else 
	{
		/* Check who sent it */
		if (sender == &downStream1)
		{
			/* Send up stream towards SFC */
			forwardUp(buffer, len);
		}
		else if (sender == &upStream1)
		{
			/* Send downstream away from SFC */
			forwardDown(buffer, len);
		}
		else
		{
			/* Throw an error */
			error_badDest(hdr_in, hdr_out, hdr_err);
			outgoingPacket[hdr_size + hdr_out->len] = computeMySum(outgoingPacket, 
															       &outgoingPacket[hdr_size+hdr_out->len]);
			upStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
		}
	}
}

/* Sorts through commands */
void commandCenter()
{
	/* Check commands */
	if (hdr_in->cmd == ePingPong)
	{
		whatToDoIfPingPong(hdr_out);
	}
	
	else if (hdr_in->cmd == eSetPriority)
	{
		whatToDoIfSetPriority(hdr_prio, hdr_out, commandPriority);
	}
	
	else if (hdr_in->cmd == eFakeSensorRead)
	{
		whatToDoIfFSR(hdr_out);
	}
	
	else if (hdr_in->cmd == eFakeError1)
	{
		error_badDest(hdr_in, hdr_out, hdr_err);
	}
	
	else if (hdr_in->cmd == eFakeError2)
	{
		error_badArgs(hdr_in, hdr_out, hdr_err);
	}
	
	else if (hdr_in->cmd == eMapDevices)
	{
		whatToDoIfMap(hdr_out, downStreamDevices, numDevices);
	}
	
	else if (hdr_in->cmd == eReset)
	{
		memset(downStreamDevices, 0, numDevices);
		memset(commandPriority, 0 , 255);
		numDevices = 0;
		// sysCtlReset();
		return;
	}
	/* Else this is not a programmed command on this device. Throw error */
	else
	{
		error_badCommand(hdr_in, hdr_out, hdr_err);
	}
	
	/* Compute checksum & send out packet */
	outgoingPacket[hdr_size+hdr_out->len] = computeMySum(outgoingPacket, 
														 &outgoingPacket[hdr_size+hdr_out->len]);
	upStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
}

/* Forwards the packet towards the SFC */
void forwardUp(const uint8_t * buffer, size_t len)
{
	/* Continue to send the message */
	upStream1.send(buffer, len);
	checkUpBoundDst();
}

/* Forwards the packet away from the SFC */
void forwardDown(const uint8_t * buffer, size_t len)
{
	if (!checkDownBoundDst()) downStream1.send(buffer,len);
}

/* If the device isn't already known, add it to the list of known devices */
void checkUpBoundDst()
{
	if (findMe(downStreamDevices, downStreamDevices + numDevices, 
			hdr_in->src) == downStreamDevices + numDevices)
	{
		downStreamDevices[numDevices] = hdr_in->src;
		numDevices++;
	}
}

/* Check if the intended device is attached */
bool checkDownBoundDst()
{
	if (findMe(downStreamDevices, downStreamDevices + numDevices, hdr_in->dst) 
				== downStreamDevices + numDevices)
	{
		/* Throw an error */
		error_badDest(hdr_in, hdr_out, hdr_err);
		outgoingPacket[hdr_size + hdr_out->len] = computeMySum(outgoingPacket, 
														       &outgoingPacket[hdr_size+hdr_out->len]);
		upStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
		return true;
	}
	else return false;
}

/* System Restart function */
void sysCtlReset( void )
{
    // An application error has occurred that cannot be recovered from.
    (*((volatile uint32_t *)NVIC_APINT)) |= NVIC_APINT_SYSRESETREQ; // Issue a System Reset
}