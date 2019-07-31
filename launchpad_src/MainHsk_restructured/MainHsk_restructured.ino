/*
 * MainHSK_prototype.ino
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

#define BAUD 1125000

/* Declare instances of PacketSerial to set up the serial lines */
PacketSerial downStream1;
PacketSerial upStream1;
PacketSerial upStream2;
PacketSerial upStream3;
PacketSerial upStream4;
PacketSerial upStream5;
PacketSerial upStream6;
PacketSerial upStream7;

/*******************************************************************************
* Defines
*******************************************************************************/
/* Name of this device */
housekeeping_id myID = eMainHsk;

/* Outgoing buffer, for up or downstream. Only gets used once a complete packet
 * is received -- a command or forward is executed before anything else happens,
 * so there shouldn't be any over-writing here. */
uint8_t outgoingPacket [MAX_PACKET_LENGTH] ={0}; 

/* Use pointers for all device's housekeeping headers */
housekeeping_hdr_t * hdr_in;     housekeeping_hdr_t * hdr_out;
housekeeping_err_t * hdr_err;   housekeeping_prio_t * hdr_prio;

/* Memory buffers for housekeeping system functions */
uint8_t numDevices = 0;           // Keep track of how many devices are upstream
uint8_t commandPriority[255] = {0};     // Each command's priority takes up one byte
PacketSerial * serialDevices[7] = {&upStream1, &upStream2, &upStream3, 
                     &upStream4, &upStream5,
                     &upStream6, &upStream7}; 
                      // Pointer to an address's serial port
uint8_t addressList[7][254] = {0};    // List of all upstream devices

/* Utility variables for internal use */
uint8_t checkin;    // Used for comparing checksum values
size_t hdr_size = sizeof(housekeeping_hdr_t)/sizeof(hdr_out->src); // size of the header
uint8_t numSends = 0; // Used to keep track of number of priority commands executed
int bus = 0;

/*******************************************************************************
* Main program
*******************************************************************************/
void setup()
{
  Serial.begin(BAUD);
  downStream1.setStream(&Serial);
  downStream1.setPacketHandler(&checkHdr);
  
//  upStream1.setStream(&Serial);
//  upStream1.setPacketHandler(&checkHdr);
  
  Serial1.begin(BAUD);
  upStream2.setStream(&Serial1);
  upStream2.setPacketHandler(&checkHdr);
  
  Serial2.begin(BAUD);
  upStream3.setStream(&Serial2);
  upStream3.setPacketHandler(&checkHdr);
  
  Serial3.begin(BAUD);
  upStream4.setStream(&Serial3);
  upStream4.setPacketHandler(&checkHdr);
  
  Serial4.begin(BAUD);
  upStream5.setStream(&Serial4);
  upStream5.setPacketHandler(&checkHdr);
  
  Serial5.begin(BAUD);
  upStream6.setStream(&Serial5);
  upStream6.setPacketHandler(&checkHdr);
  
  Serial7.begin(BAUD);
  upStream7.setStream(&Serial7);
  upStream7.setPacketHandler(&checkHdr);

  // Point to data in a way that it can be read as a header
  hdr_out = (housekeeping_hdr_t *) outgoingPacket;
  hdr_err = (housekeeping_err_t *) (outgoingPacket + hdr_size);
  
}

/*******************************************************************************
 * Main program
 ******************************************************************************/
void loop()
{
  /* Continuously read in one byte at a time until a packet is received */
  if (downStream1.update() != 0) badPacketReceived(&downStream1);
  if (upStream1.update() != 0) badPacketReceived(&upStream1);
  if (upStream2.update() != 0) badPacketReceived(&upStream2);
  if (upStream3.update() != 0) badPacketReceived(&upStream3);
  if (upStream4.update() != 0) badPacketReceived(&upStream4);
  if (upStream5.update() != 0) badPacketReceived(&upStream5);
  if (upStream6.update() != 0) badPacketReceived(&upStream6);
  if (upStream7.update() != 0) badPacketReceived(&upStream7);
}

void checkHdr(const void * sender, const uint8_t * buffer, size_t len) { 
  // Default header & error data values
  hdr_out->src = myID;          // Source of data packet
  hdr_in = (housekeeping_hdr_t *) buffer; // Point to the incoming buffer
  hdr_prio = (housekeeping_prio_t *) (buffer + hdr_size);
  // If an error occurs at this device from a message
  if (hdr_in->dst == eBroadcast) hdr_err->dst = myID;
  else hdr_err->dst = hdr_in->dst;
 // If the checksum didn't match, throw a bad args error
    // Check for data corruption
  if (!(verifyChecksum((uint8_t *) buffer))) {
      error_badArgs(hdr_in, hdr_out, hdr_err);  
      fillChecksum((uint8_t *) outgoingPacket);
      downStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
  }
  else {
  // Check if the message is for this device and not a broadcast
    if (hdr_in->dst == myID && hdr_in->dst != eBroadcast) {
      // everything below this is now commented and can be reverted back but I am overhauling the way in which commands are handled.
      // these functions need to point to the data portion of the packet to put the read or write return there 
      // function handleLocalCommand will call localWrite or localRead 
      if(hdr_in->cmd==eTestMode) handleTestMode(hdr_in, (uint8_t *) hdr_in + hdr_size, (uint8_t *) outgoingPacket);
      else handleLocalCommand(hdr_in, (uint8_t *) (hdr_in + hdr_size), (uint8_t *) outgoingPacket); // this constructs the outgoingpacket when its a localcommand and sends the packet and patrick wants this sort of structure.
      // what to do after local command is run?
      // if it is a set priority command run handlePriority
      // if the command is eBroadcast, also need to send the command up to other devices
      // Forward downstream if eBroadcast
    } 
    else if (hdr_in->dst == eBroadcast) {
      upStream1.send(buffer, len);
      upStream2.send(buffer, len);
      upStream3.send(buffer, len);
      upStream4.send(buffer, len);
      upStream5.send(buffer, len);
      upStream6.send(buffer, len);
      upStream7.send(buffer, len);
    }
  // If the message wasn't meant for this device pass it along (up is away from SFC and down and is to SFC
    else {
      if (sender == &downStream1) forwardUp(buffer, len); 
      else forwardDown(buffer, len, sender);
    }    
  }
}
 // handle priorities later
      // If a send all priority command is received
/*      if ((int) hdr_in->cmd < 253 && (int) hdr_in->cmd >= 250)
//      {
        numSends = 0;
        // Execute all commands of that type 
        for (int i = 0; i < 255; i++)
        {
          if (commandPriority[i] == (int) hdr_in->cmd -249)
          {
            hdr_in->cmd = (uint8_t) i;
            commandCenter();
            numSends++;
          }
        }
        // If there are no commands of that type & the destination isn't eBroadcast
        if (checkThatPriority(hdr_in, hdr_out, numSends))
        {
          fillChecksum((uint8_t *) outgoingPacket);
          downStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
        }
      }
*/
      // If a send all command is received (eSendAll is NOT a local command
// finish this later
/*
      else if (hdr_in->cmd == eSendAll)
      {
        for (int i = FIRST_LOCAL_COMMAND; i < LAST_LOCAL_COMMAND; i++)
        {
          hdr_in->cmd = (uint8_t) i;
          handleLocalCommand(hdr_in, (uint8_t *) hdr_in + hdr_size, (uint8_t *) outgoingPacket);
        }
      }
    }
*/  
 // this void function is redudant with patricks handlLocalCommand
/*
void commandCenter()
{
  // Check commands
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

  else if (hdr_in->cmd == eTestMode)
  {
    uint16_t numTestPackets = 0; // to get to the data bytes
    uint16_t *numTestPackets_p = &numTestPackets;
    // if the length of data attached is not zero then go into testmode and send the number of packets that was sent with the command 
    if(hdr_in->len == 2){
      // bitshift the first data byte to be an extra byte long then bitwise or with the next data byte.
      numTestPackets= ((uint16_t) *((uint8_t *) hdr_in + 4 + 1) << 8) | (uint8_t) *((uint8_t *) hdr_in + 4);
      while (numTestPackets){
        whatToDoIfTestMode(numTestPackets_p, hdr_out);
        numTestPackets--;
        // do a send (after numtestpackets is 0 then will go on to the other send). 
        fillChecksum((uint8_t *) outgoingPacket);
        downStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
      }
    }
    // else send bad length error
    else{
      // build error header and send that?
    }
  }
  
  else if (hdr_in->cmd == eReset)
  {
    SysCtlReset();
  }
  // Else this is not a programmed command on this device. Throw error
  else
  {
    error_badCommand(hdr_in, hdr_out, hdr_err);
  }
  
  //  checksum & send out packet 
  fillChecksum((uint8_t *) outgoingPacket);
  downStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
}
*/


/* Function flow:
 * --Forwards the packet towards the SFC 
 * 
 * Function params:
 * buffer:    The decoded packet received
 * len:     Size (in bytes) of the incoming packet above
 * sender:    PacketSerial instance (serial line) where the message was received
 * 
 */
void forwardDown(const uint8_t * buffer, size_t len, const void * sender)
{
  /* Continue to send the message */
  downStream1.send(buffer, len);
  checkDownBoundDst(sender);
}

/* Function flow:
 * --Checks if the intended destination is a device known to be attached
 *    --If it is known, send the packet down the correct serial line towards that device
 *    --If it isn't, throw an error
 * 
 * Function params:
 * buffer:    The decoded packet received
 * len:     Size (in bytes) of the incoming packet above
 * 
 */
void forwardUp(const uint8_t * buffer, size_t len)
{
  bus = checkUpBoundDst();
  if (bus) serialDevices[bus]->send(buffer,len);
}

/* Function flow:
 * --Checks to see if the downstream device that sent the message is known
 *    --If not, add it to the list of known devices
 *    --If yes, just carry on
 * --Executed every time a packet is received from downStream
 * 
 * Function params:
 * sender:    PacketSerial instance (serial line) where the message was received
 * 
 */
void checkDownBoundDst(const void * sender)
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
 *    --In the array 'serialDevices'
 * --If the device is not known, an error is thrown
 * 
 */
int checkUpBoundDst()
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
  fillChecksum((uint8_t *) outgoingPacket);
  downStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
  return 0;
}

/* Function flow:
 * --Find the device address that produced the error
 * --Execute the bad length function & send the error to the SFC
 * 
 * Function params:
 * sender:    PacketSerial instance which triggered the error protocol
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
  fillChecksum((uint8_t *) outgoingPacket);
  downStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
}




  /* PSA code goes like this, hdr_in->len!=0 means local write, hdr_in->len==0 means local read */
  // Fn to handle a local command write.
// This gets called when a local command is received
// with data (len != 0).

int handleLocalWrite(uint8_t localCommand, uint8_t *data, uint8_t len) {
  switch(localCommand) {
  eTestHeaterControl:
    return whatToDoIfTestHeaterControl((uint8_t*) data, (uint8_t) len);
  eHeaterControl:
    return whatToDoIfHeaterControl((uint8_t*) data, (uint8_t) len);
    // construct a header out and put the output there, if not the right value report error?
//  eAutoPriorityPeriod:
  // got an autoPriorityPeriod command
//    return setAutoPriorityPeriods(data, len);
//  ePacketCount:
//    return -EBADLEN;
  default:
    return -EBADCOMMAND;    
  }
}

int handleLocalRead(uint8_t localCommand, uint8_t *buffer) {
  int retval = 0;
  switch(localCommand) {
  case ePingPong:
    whatToDoIfPingPong(buffer);
    retval=0;
    break;
  case eIntSensorRead: 
    whatToDoIfISR(buffer);
    retval=4;
    break;
//  eAutoPriorityPeriod:
//    memcpy(buffer, (uint8_t *) currentAutoPriorityPeriods, sizeof(currentAutoPriorityPeriods));
//    return sizeof(currentAutoPriorityPeriods);
//  ePacketCount:
//    memcpy(buffer, (uint8_t *) currentPacketCount, sizeof(currentPacketCount));
//    return sizeof(currentPacketCount);
  default:
    retval=EBADCOMMAND;
  }  
  return retval;
}

void buildError(housekeeping_err_t *err, housekeeping_hdr_t *respHdr, housekeeping_hdr_t * hdr, int error){
  respHdr->cmd = eError;
  respHdr->len = 4;
  err->src = hdr->src;
  err->dst = hdr->dst;
  err->cmd = hdr->cmd;
  err->error = error;
}
// Function to call first when localcommand sent. 
// Store the result as retval (which should be bytes read or written?)
void handleLocalCommand(housekeeping_hdr_t *hdr, uint8_t *data, uint8_t * responsePacketBuffer) {
  int retval=0;
  housekeeping_hdr_t *respHdr = (housekeeping_hdr_t *) responsePacketBuffer;
  uint8_t *respData = responsePacketBuffer + sizeof(housekeeping_hdr_t);
  respHdr->src = myID;
  respHdr->dst = hdr->src;
  if (hdr->len) {
    retval = handleLocalWrite(hdr->cmd, data, hdr->len); // retval is negative construct the baderror hdr and send that instead. 
    if(retval==0) {
      respHdr->cmd = hdr->cmd;
      respHdr->len = retval; // response bytes of the write.
    }
    else{
      housekeeping_err_t *err = (housekeeping_err_t *) respData;
      buildError(err, respHdr, hdr, retval);
    }  
  } 
  else {
    // local read. by definition these always go downstream.
    retval = handleLocalRead(hdr->cmd, respData);
    if (retval>=0) {
      uint8_t test_retval = retval;
      respHdr->cmd = hdr->cmd;
      respHdr->len = test_retval; //bytes read
    }
    else {
      housekeeping_err_t *err = (housekeeping_err_t *) respData;
      buildError(err, respHdr, hdr, retval); // the err pointer is pointing to the data of the response packet based on the line above so this fn fills that packet. 
    }
    fillChecksum(responsePacketBuffer);
    // send to SFC
    downStream1.send(responsePacketBuffer, respHdr->len + sizeof(housekeeping_hdr_t) + 1 );
  }
}
void handleTestMode(housekeeping_hdr_t *hdr, uint8_t *data, uint8_t * responsePacketBuffer) {
  housekeeping_hdr_t *respHdr = (housekeeping_hdr_t *) responsePacketBuffer;
  uint8_t *respData = responsePacketBuffer + sizeof(housekeeping_hdr_t);
  respHdr->src = myID;
  respHdr->dst = hdr->src;
// if length was actually placed then go into testmode, else build badlength error.
  if (hdr->len) {
   //construct data incoming to be the num testpackets and send the data packet in a while loop and decrement numtestpackets?
    uint16_t numTestPackets = ((uint16_t) (*(data) << 8)) | *(data+1) ; // figure out the correct way to get 2 bytes into a 16_t
    while(numTestPackets){
      *respData = numTestPackets;
      respHdr->cmd = hdr->cmd;
      respHdr->len = 0x02; // response bytes of the write.
      fillChecksum(responsePacketBuffer);
      // send to SFC
      downStream1.send(responsePacketBuffer, respHdr->len + sizeof(housekeeping_hdr_t) + 1 );  
      numTestPackets--;
    }
  }
  else{
    housekeeping_err_t *err = (housekeeping_err_t *) respData;
    buildError(err, respHdr, hdr, -3); // -3 for EBADLEN
    fillChecksum(responsePacketBuffer);
    // send to SFC
    downStream1.send(responsePacketBuffer, respHdr->len + sizeof(housekeeping_hdr_t) + 1 );  
  }  
}

/* SPW desires
 *  ok - working fine.   Are you taking requests?

If so, here they are, in order of desirability:
 - count the number of packets you send and encode that data as the first 4 bytes of the eMainHsk packets.  
 - add a random number of additional random bytes to the packets.  Dunno what the max packet size will be, but maybe 0-1kb?
 - add a few random packets from other sources (ids 2,3,4,5)

If you really want to go crazy, you could have the board turn on and off packet sending, based on commands that I send it.  So, it would normally start up in the off condition.  If it receives a start command, it starts sending.  If it receives a stop command, it stops.  Picking some random command IDs, the headers would be:
Start => [0x01][0xFC][0xFD][0x00]
Stop => [0x01][0xFC][0xFE][0x00]

This would provide a nice baseline test of the 2-way comm protocols.

-S
 */
