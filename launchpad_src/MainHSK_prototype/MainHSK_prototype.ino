#include <PacketSerial.h>
#include <iProtocol.h>
#include <CommandResponse.h>

/* Declare an instance of PacketSerial to set up the serial bus */
PacketSerial upStream1;
PacketSerial downStream1;

/*******************************************************************************
* Defines
*******************************************************************************/

/* Name this device */
housekeeping_id myID = eMainHsk;

/* Declarations to keep track of device list */
uint8_t downStreamDevices[254] = {0};	// Keep a list of downstream devices
uint8_t numDevices = 0;			// Keep track of how many devices are downstream

/* Create buffers for data */
uint8_t outgoingPacket [MAX_PACKET_LENGTH]; 	// Buffer for outgoing packet
uint8_t incomingPacket [MAX_PACKET_LENGTH];	// Buffer for incoming packet

/* Guess: defining the variable here makes the linker connect where these
 * 		  variables are being used 	*/
housekeeping_hdr_t * hdr_in;
housekeeping_hdr_t * hdr_out;

/* Checksum values */
uint8_t checkin;

/* Error message */
uint8_t error[] = "Error";

/*******************************************************************************
* Main program
*******************************************************************************/
void setup()
{
	Serial.begin(9600);
	upStream1.setStream(&Serial);
	upStream1.setPacketHandler(&isItMe);
  
	Serial1.begin(9600);
	downStream1.setStream(&Serial1);
	downStream1.setPacketHandler(&isItMe);
	
	/* Point to data in a way that it can be read as a header_hdr_t */
	hdr_in = (housekeeping_hdr_t *) incomingPacket;	
	hdr_out = (housekeeping_hdr_t *) outgoingPacket;
}

void loop()
{
	/* Continuously read in one byte at a time until a packet is received */
	upStream1.update();
	downStream1.update();
}

/*******************************************************************************
* Functions
*******************************************************************************/
/* To be executed when a packet is received */
void isItMe(const void * sender, const uint8_t * buffer, size_t len)
{
    /* Read in the header */
	hdr_in->src = buffer[0];
	hdr_in->dst = buffer[1];
	hdr_in->cmd = buffer[2];
	hdr_in->len = buffer[3];
	
	/* Check if the message was intended for this device
	 * If it was, check and execute the command  */
	if (hdr_in->dst == myID)
	{
		/* Check for data corruption */
		checkin = computeMySum(buffer, buffer[4 + hdr_in->len]);
		if (checkMySum(checkin, buffer[4 + hdr_in->len]))
		{
			commandCentral(buffer);
		}
	}
  
	/* If it wasn't, pass along the header */
	else 
	{
		/* Check who sent it (maybe?) */
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
			upStream1.send(error,5);
			return;
		}
	}
}

/* Sorts through commands */
void commandCentral(const uint8_t * packet)
{
	/* Create the header */
	hdr_out->src = myID;	// Source of data packet
	
	/* Check commands */
	if (hdr_in->cmd == ePingPong)
	{
		whatToDoIfPingPong(hdr_out);
	}
	
	if (hdr_in->cmd == eFakeSensorRead)
	{
		whatToDoIfFSR(hdr_out);
	}
	
	/* Compute checksum */
	outgoingPacket[4+hdr_out->len] = computeMySum(outgoingPacket, 
										   outgoingPacket[4+hdr_out->len]);
	/* Send out */
	upStream1.send(outgoingPacket, 
					sizeof(housekeeping_hdr_t)/sizeof(hdr_out->src) + hdr_out->len + 1);
}

/* Forwards the packet towards the SFC */
void forwardUp(const uint8_t * buffer, size_t len)
{
	/* Continue to send the message */
	upStream1.send(buffer, len);
	
	/* If the device isn't already known, add it to the list */
	if (findMe(downStreamDevices, downStreamDevices + numDevices, 
			hdr_in->src) == downStreamDevices + numDevices)
	{
		downStreamDevices[numDevices] = hdr_in->src;
		numDevices++;
	}
}

/* Forwards the packet away from the SFC */
void forwardDown(const uint8_t * buffer, size_t len)
{
	downStream1.send(buffer,len);
}