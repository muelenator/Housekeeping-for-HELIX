/*
 * DCTHSK_prototype.ino
 * 
 * Initiates serial ports & follows HSK protocol for command responses and error
 * reporting. This program can be used on other devices by changing the device
 * address (myID) and the upStream serial connection (direct line to the SFC)
 *
 * 
 */

#include <PacketSerial.h>
#include <iProtocol.h>
#include <CommandResponse.h>

#include <driverlib/sysctl.h>

#define BAUD 1250000

/* Declare instances of PacketSerial to set up the serial lines */
PacketSerial upStream1;
PacketSerial downStream1;
PacketSerial downStream2;
PacketSerial downStream3;
PacketSerial downStream4;
PacketSerial downStream5;
PacketSerial downStream6;
PacketSerial downStream7;

/*******************************************************************************
* Defines
*******************************************************************************/
/* Name of this device */
housekeeping_id myID = eDCTHsk;

/* Outgoing buffer, for up or downstream. Only gets used once a complete packet
 * is received -- a command or forward is executed before anything else happens,
 * so there shouldn't be any over-writing here. */
uint8_t outgoingPacket [MAX_PACKET_LENGTH]; 	// Outgoing Buffer

/* Use pointers for all device's housekeeping headers */
housekeeping_hdr_t * hdr_in; 		housekeeping_hdr_t * hdr_out;
housekeeping_err_t * hdr_err;		housekeeping_prio_t * hdr_prio;

/* Memory buffers for housekeeping system functions */
uint8_t numDevices = 0;						// Keep track of how many devices are downstream
uint8_t commandPriority[255] = {0};			// Each command's priority takes up one byte
PacketSerial * serialDevices[7] = {&downStream1, &downStream2, &downStream3, 
									   &downStream4, &downStream5,
									   &downStream6, &downStream7};	
									    // Pointer to an address's serial port
uint8_t addressList[7][254] = {0}; 		// List of all downstream devices

/* Utility variables for internal use */
uint8_t checkin;		// Used for comparing checksum values
size_t hdr_size = sizeof(housekeeping_hdr_t)/sizeof(hdr_out->src); // size of the header
uint8_t numSends = 0;	// Used to keep track of number of priority commands executed
int bus = 0;

/*******************************************************************************
* Main program
*******************************************************************************/
void setup()
{
	Serial1.begin(BAUD);
	upStream1.setStream(&Serial1);
	upStream1.setPacketHandler(&checkHdr);
	
	Serial.begin(BAUD);
	downStream1.setStream(&Serial);
	downStream1.setPacketHandler(&checkHdr);
  
	downStream2.setStream(&USBSerial);
	downStream2.setPacketHandler(&checkHdr);
	
	Serial2.begin(BAUD);
	downStream3.setStream(&Serial2);
	downStream3.setPacketHandler(&checkHdr);
	
	Serial3.begin(BAUD);
	downStream4.setStream(&Serial3);
	downStream4.setPacketHandler(&checkHdr);
	
	Serial4.begin(BAUD);
	downStream5.setStream(&Serial4);
	downStream5.setPacketHandler(&checkHdr);
	
	Serial5.begin(BAUD);
	downStream6.setStream(&Serial5);
	downStream6.setPacketHandler(&checkHdr);
	
	Serial7.begin(BAUD);
	downStream7.setStream(&Serial7);
	downStream7.setPacketHandler(&checkHdr);

	/* Point to data in a way that it can be read as a header */
	hdr_out = (housekeeping_hdr_t *) outgoingPacket;
	hdr_err = (housekeeping_err_t *) (outgoingPacket + hdr_size);
	
}

/*******************************************************************************
 * Main program
 ******************************************************************************/
void loop()
{
	/* Continuously read in one byte at a time until a packet is received */
	if (upStream1.update() != 0) badPacketReceived(&upStream1);
	if (downStream1.update() != 0) badPacketReceived(&downStream1);
	if (downStream2.update() != 0) badPacketReceived(&downStream2);
	if (downStream3.update() != 0) badPacketReceived(&downStream3);
	if (downStream4.update() != 0) badPacketReceived(&downStream4);
	if (downStream5.update() != 0) badPacketReceived(&downStream5);
	if (downStream6.update() != 0) badPacketReceived(&downStream6);
	if (downStream7.update() != 0) badPacketReceived(&downStream7);
}

/*******************************************************************************
* Functions
*******************************************************************************/
/* To be executed when a packet is received 
 *
 * Function flow:
 * --Creates default header & error header values that most board functions will use
 * --Checks if the message was intended for this device
 * 		--If so, check if it was a send priority command
 * 			--If not, execute the command by calling CommandCenter().
 * 		  	  priority request, 
 * 		--If not, forward on the packet based on where it came from
 * 
 * Function params:
 * sender:		PacketSerial instance where the message came from
 * buffer:		The decoded packet
 * len:			The size (in bytes) of the decoded packet
 * 
 * 
 * */
void checkHdr(const void * sender, const uint8_t * buffer, size_t len)
{	
	/* Default header & error data values */
	hdr_out->src = myID;					// Source of data packet
	hdr_in = (housekeeping_hdr_t *) buffer;	// Point to the incoming buffer
	hdr_prio = (housekeeping_prio_t *) (buffer + hdr_size);
	
	
	/* If an error occurs at this device from a message	*/
	if (hdr_in->dst == eBroadcast) hdr_err->dst = myID;
	else hdr_err->dst = hdr_in->dst;
	
	/* Check if the message is for this device. If so, check & execute command */
	if (hdr_in->dst == myID || hdr_in->dst == eBroadcast)
	{
		/* Check for data corruption */
		checkin = computeMySum(buffer, &buffer[len - 1]);
		
		if (checkMySum(checkin, buffer[len - 1]))
		{
			/* Check for bad length	*/
			if (hdr_in->len != len - 4 - 1)
			{
				badPacketReceived((PacketSerial *) sender);
				return;
			}
			
			/* Forward downstream if eBroadcast */
			if (hdr_in->dst == eBroadcast)
			{
				downStream1.send(buffer, len);
				downStream2.send(buffer, len);
				downStream3.send(buffer, len);
				downStream4.send(buffer, len);
				downStream5.send(buffer, len);
				downStream6.send(buffer, len);
				downStream7.send(buffer, len);
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
		if (sender == &upStream1)
		{
			/* Send downstream away from SFC */
			forwardDown(buffer, len);
		}
		/* Send up stream towards SFC */
		else forwardUp(buffer, len, sender);
	}
}

/* Function flow:
 * --Sorts through commands 
 * --If a command is not known, send an error
 * 
 */
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
	
	else if (hdr_in->cmd == eIntSensorRead)
	{
		whatToDoIfISR(hdr_out);
	}
	
	else if (hdr_in->cmd == eMapDevices)
	{
		whatToDoIfMap(hdr_out, addressList, numDevices);
	}
	
	else if (hdr_in->cmd == eReset)
	{
		SysCtlReset();
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

/* Function flow:
 * --Forwards the packet towards the SFC 
 * 
 * Function params:
 * buffer:		The decoded packet received
 * len:			Size (in bytes) of the incoming packet above
 * sender:		PacketSerial instance (serial line) where the message was received
 * 
 */
void forwardUp(const uint8_t * buffer, size_t len, const void * sender)
{
	/* Continue to send the message */
	upStream1.send(buffer, len);
	checkUpBoundDst(sender);
}

/* Function flow:
 * --Checks if the intended destination is a device known to be attached
 * 		--If it is known, send the packet down the correct serial line towards that device
 * 		--If it isn't, throw an error
 * 
 * Function params:
 * buffer:		The decoded packet received
 * len:			Size (in bytes) of the incoming packet above
 * 
 */
void forwardDown(const uint8_t * buffer, size_t len)
{
	bus = checkDownBoundDst();
	if (bus) serialDevices[bus]->send(buffer,len);
}

/* Function flow:
 * --Checks to see if the downstream device that sent the message is known
 * 		--If not, add it to the list of known devices
 * 		--If yes, just carry on
 * --Executed every time a packet is received from downStream
 * 
 * Function params:
 * sender:		PacketSerial instance (serial line) where the message was received
 * 
 */
void checkUpBoundDst(const void * sender)
{
	for (int i=0; i<7; i++)
	{
		if (serialDevices[i] == (PacketSerial *) sender)
		{
			if (addressList[i][hdr_in->src] == 0)
			{
				addressList[i][hdr_in->src] = (uint8_t) hdr_in->src;
				numDevices++;
				return;
			}
		}
	}
}

/* Function flow:
 * --Checks each serial line's list of devices for the intended destination of a packet
 * --If a line has that device known, a number corresonding to that serial line is returned
 * 		--In the array 'serialDevices'
 * --If the device is not known, an error is thrown
 * 
 */
int checkDownBoundDst()
{
	for (int i=0; i<7; i++)
	{
		if (addressList[i][hdr_in->dst] != 0)
		{
			return i;
		}
	}

	/* If it wasn't found, throw an error */
	error_badDest(hdr_in, hdr_out, hdr_err);
	outgoingPacket[hdr_size + hdr_out->len] = computeMySum(outgoingPacket, 
															&outgoingPacket[hdr_size+hdr_out->len]);
	upStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
	return 0;
}

/* Function flow:
 * --Find the device address that produced the error
 * --Execute the bad length function & send the error to the SFC
 * 
 * Function params:
 * sender:		PacketSerial instance which triggered the error protocol
 * 
 * 
 * Send an error if a packet is unreadable in some way */
void badPacketReceived(PacketSerial * sender)
{
	int i = 0;
	for (i=0; i < 7; i++)
	{
		if (sender == serialDevices[i])
		{
			hdr_in->src = addressList[i][0];
			break;	
		}
	}

	if (i==7) hdr_in->src = eSFC;

	hdr_out->src = myID;
	error_badLength(hdr_in, hdr_out, hdr_err);
	outgoingPacket[hdr_size + hdr_out->len] = computeMySum(outgoingPacket, 
														       &outgoingPacket[hdr_size+hdr_out->len]);
	upStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
}
