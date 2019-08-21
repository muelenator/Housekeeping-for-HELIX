/*
 * DCTHSK_prototype.ino
 * 
 * Initiates serial ports & follows HSK protocol for command responses and error
 * reporting. This program can be used on other devices by changing the device
 * address (myID) and the upStream serial connection (direct line to the SFC)
 *
 * 
 */

#include <Core_protocol.h>
#include <PacketSerial.h>
#include <driverlib/sysctl.h>

/* These are device specific */
#include "src/DCTHSK_lib/DCTHSK_protocol.h"
#include "src/DCTHSK_lib/DCTHSK_support_functions.h"
#define DOWNBAUD 115200 // Baudrate to the SFC
#define UPBAUD 115200    // Baudrate to upsteam devices
#define TEST_MODE_PERIOD 100 // period in milliseconds between testmode packets being sent
#define FIRST_LOCAL_COMMAND 2 // value of hdr->cmd that is the first command local to the board
#define NUM_LOCAL_CONTROLS 6 // how many commands total are local to the board

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
housekeeping_id myID = eDCTHsk;

/* Outgoing buffer, for up or downstream. Only gets used once a complete packet
 * is received -- a command or forward is executed before anything else happens,
 * so there shouldn't be any over-writing here. */
uint8_t outgoingPacket [MAX_PACKET_LENGTH] ={0}; 

/* Use pointers for all device's housekeeping headers and the autopriorityperiods*/
housekeeping_hdr_t * hdr_in;     housekeeping_hdr_t * hdr_out;
housekeeping_err_t * hdr_err;   housekeeping_prio_t * hdr_prio;
/* Memory buffers for housekeeping system functions */
uint8_t numDevices = 0;           // Keep track of how many devices are upstream
uint8_t commandPriority[NUM_LOCAL_CONTROLS] = {0};     // Each command's priority takes up one byte
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
uint16_t currentPacketCount=0;
unsigned long timelastpacket;
/*******************************************************************************
* Main program
*******************************************************************************/
void setup()
{
  Serial.begin(DOWNBAUD);
  downStream1.setStream(&Serial);
  downStream1.setPacketHandler(&checkHdr);
  
//  upStream1.setStream(&Serial);
//  upStream1.setPacketHandler(&checkHdr);
  
  Serial1.begin(UPBAUD);
  upStream2.setStream(&Serial1);
  upStream2.setPacketHandler(&checkHdr);
  
  Serial2.begin(UPBAUD);
  upStream3.setStream(&Serial2);
  upStream3.setPacketHandler(&checkHdr);
  
  Serial3.begin(UPBAUD);
  upStream4.setStream(&Serial3);
  upStream4.setPacketHandler(&checkHdr);
  
  Serial4.begin(UPBAUD);
  upStream5.setStream(&Serial4);
  upStream5.setPacketHandler(&checkHdr);
  
  Serial5.begin(UPBAUD);
  upStream6.setStream(&Serial5);
  upStream6.setPacketHandler(&checkHdr);
  
  Serial7.begin(UPBAUD);
  upStream7.setStream(&Serial7);
  upStream7.setPacketHandler(&checkHdr);

  // Point to data in a way that it can be read as a header
  hdr_out = (housekeeping_hdr_t *) outgoingPacket;
  hdr_err = (housekeeping_err_t *) (outgoingPacket + hdr_size);
  currentPacketCount=0;
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
  if (hdr_in->dst == eBroadcast || hdr_in->dst==myID) hdr_err->dst = myID;
  else hdr_err->dst = hdr_in->dst;
 // If the checksum didn't match, throw a bad args error
    // Check for data corruption
  if (!(verifyChecksum((uint8_t *) buffer))) {
//      error_badArgs(hdr_in, hdr_out, hdr_err);  
      buildError(hdr_err, hdr_out, hdr_in, EBADARGS);
      fillChecksum((uint8_t *) outgoingPacket);
      downStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
      currentPacketCount++;
  }
  else {
  // Check if the message is a broadcast or local command and only then execute it. 
    if (hdr_in->dst == eBroadcast || hdr_in->dst==myID) {
      if(hdr_in->cmd==eTestMode) handleTestMode(hdr_in, (uint8_t *) hdr_in + hdr_size, (uint8_t *) outgoingPacket);
      else if ((int)(hdr_in->cmd < 254) && (int)(hdr_in->cmd > 249)) handlePriority(hdr_in, (uint8_t *) outgoingPacket); // for doing a send of priority type.
      else handleLocalCommand(hdr_in, (uint8_t *) hdr_in + hdr_size, (uint8_t *) outgoingPacket); // this constructs the outgoingpacket when its a localcommand and sends the packet.
    } 
  // If the message wasn't meant for this device pass it along (up is away from SFC and down and is to SFC
    else {
      if (sender == &downStream1) forwardUp(buffer, len); 
      else forwardDown(buffer, len, sender);
    }
  }
}
void forwardDown(const uint8_t * buffer, size_t len, const void * sender)
{
  /* Continue to send the message */
  downStream1.send(buffer, len);
  checkDownBoundDst(sender);
  currentPacketCount++;
}
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
int checkUpBoundDst(){
  for (int i=0; i<7; i++){
    if (addressList[i][hdr_in->dst] != 0){
      return i;
    }
  }
  /* If it wasn't found, throw an error */
//  Building Error is a process right now..
  housekeeping_hdr_t *hdr_out = (housekeeping_hdr_t *)outgoingPacket;
  uint8_t *respData = outgoingPacket + hdr_size;
  housekeeping_err_t *err = (housekeeping_err_t *) respData;
  buildError(err, hdr_out, hdr_in, EBADDEST); 
  fillChecksum((uint8_t *) outgoingPacket);
  downStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
  currentPacketCount++;
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
void badPacketReceived(PacketSerial * sender){
  for (int i=0; i < 7; i++){
    if (sender == serialDevices[i]){
      hdr_in->src = addressList[i][0];
      break;  
    }
    if(i==6) hdr_in->src = eSFC;
  }
  hdr_out->src = myID;

//  error_badLength(hdr_in, hdr_out, hdr_err);
  buildError(hdr_err, hdr_out, hdr_in, EBADLEN);
  fillChecksum((uint8_t *) outgoingPacket);
  downStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
  currentPacketCount++;
}
// got a priority request from destination dst
void handlePriority(housekeeping_hdr_t * hdr_in, uint8_t * responsePacketBuffer){
  housekeeping_hdr_t *respHdr = (housekeeping_hdr_t *) responsePacketBuffer;
  uint8_t *respData = responsePacketBuffer + hdr_size;
  int priority=0;
//  int retval;
  respHdr->src = myID;
  respHdr->dst = hdr_in->src; 
//  respHdr->cmd = hdr_in->cmd;
  // priority == 4 when this function is called is code for "eSendAll"
  // otherwise priority=1,2,3 and that maps to eSendLowPriority+priority
  // go through every priority
  if(hdr_in->cmd==eSendAll) priority=4;
  else priority = hdr_in->cmd - 249;
  for (int i=0;i<NUM_LOCAL_CONTROLS;i++) {
    if (commandPriority[i] == (uint8_t)priority || priority==4) {
      respHdr->cmd = (uint8_t) i + FIRST_LOCAL_COMMAND;
      respHdr->len = handleLocalRead((uint8_t) i + FIRST_LOCAL_COMMAND, respData);
      fillChecksum(responsePacketBuffer);
      downStream1.send(responsePacketBuffer, respHdr->len + hdr_size + 1);
      currentPacketCount++;
    }
  }
}

void setCommandPriority(housekeeping_prio_t * prio, uint8_t * respData, uint8_t len) {
//  housekeeping_prio_t * set_prio = (housekeeping_prio_t *) prio;
  commandPriority[prio->command-FIRST_LOCAL_COMMAND] = (uint8_t) prio->prio_type;
  memcpy(respData, (uint8_t*)prio, len);
}

// Fn to handle a local command write.
// This gets called when a local command is received
// with data (len != 0).
int handleLocalWrite(uint8_t localCommand, uint8_t * data, uint8_t len, uint8_t * respData) {
  int retval = 0;
  switch(localCommand) {
  case eSetPriority:
    setCommandPriority((housekeeping_prio_t *)data,respData,len);
//    memcpy((uint8_t*)respData, (uint8_t*)data, len);
    retval=len;
    break;
  case eHeaterControl:
    retval = whatToDoIfHeaterControl((uint8_t*) data, (uint8_t) len);
    break;
  case eTestHeaterControl:
    retval = whatToDoIfTestHeaterControl((uint8_t*) data, (uint8_t) len, respData);
    break;
  case ePacketCount:
    retval = EBADLEN;
    break;
  default:
    retval=EBADCOMMAND;    
    break;
  }
  return retval;
}

int handleLocalRead(uint8_t localCommand, uint8_t *buffer) {
  int retval = 0;
  switch(localCommand) {
  case ePingPong:
    retval=0;
    break;
  case eIntSensorRead: 
    whatToDoIfISR(buffer);
    retval=4;
    break;
  case eThermistors:
    retval = whatToDoIfThermistors(buffer);
    break;
  case ePacketCount:
    memcpy(buffer, (uint8_t *) &currentPacketCount, sizeof(currentPacketCount));
    retval = sizeof(currentPacketCount);
    break;
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
void handleLocalCommand(housekeeping_hdr_t *hdr, uint8_t * data, uint8_t * responsePacketBuffer) {
  int retval=0;
  housekeeping_hdr_t *respHdr = (housekeeping_hdr_t *) responsePacketBuffer;
  uint8_t *respData = responsePacketBuffer + sizeof(housekeeping_hdr_t);
  respHdr->src = myID;
  respHdr->dst = hdr->src;
  if (hdr->len) {
    retval = handleLocalWrite(hdr->cmd, data, hdr->len, respData); // retval is negative construct the baderror hdr and send that instead. 
    if(retval>=0) {
//      *respData= 5;
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
      respHdr->cmd = hdr->cmd;
      respHdr->len = retval; //bytes read
    }
    else {
      housekeeping_err_t *err = (housekeeping_err_t *) respData;
      buildError(err, respHdr, hdr, retval); // the err pointer is pointing to the data of the response packet based on the line above so this fn fills that packet. 
    }
  }
  fillChecksum(responsePacketBuffer);
  // send to SFC
  downStream1.send(responsePacketBuffer, respHdr->len + hdr_size + 1 );
  currentPacketCount++;
}

void handleTestMode(housekeeping_hdr_t *hdr, uint8_t *data, uint8_t * responsePacketBuffer) {
  housekeeping_hdr_t *respHdr = (housekeeping_hdr_t *) responsePacketBuffer;
  uint8_t *respData = responsePacketBuffer + sizeof(housekeeping_hdr_t);
  respHdr->src = myID;
  respHdr->dst = hdr->src;
// if length was actually placed then go into testmode, else build badlength error.
  if (hdr->len) {
   //construct data incoming to be the num testpackets and send the data packet in a while loop and decrement numtestpackets?
    uint16_t numTestPackets = ((uint16_t) (*(data+1) << 8)) | *(data) ; // figure out the correct way to get 2 bytes into a 16_t
    timelastpacket = millis();
    while(numTestPackets){
      if(long (millis()-timelastpacket)>0) { // only send every 50 milliseconds?
        *(respData) = numTestPackets;    
        *(respData+1) = numTestPackets >> 8;
        respHdr->cmd = hdr->cmd;
        respHdr->len = 0x02; // response bytes of the write.
        fillChecksum(responsePacketBuffer);
        // send to SFC
        downStream1.send(responsePacketBuffer, respHdr->len + sizeof(housekeeping_hdr_t) + 1 );  
        numTestPackets--;
        timelastpacket = timelastpacket+TEST_MODE_PERIOD;
        currentPacketCount++;
      }
    }
  }
  else{
    housekeeping_err_t *err = (housekeeping_err_t *) respData;
    buildError(err, respHdr, hdr, EBADLEN); 
    fillChecksum(responsePacketBuffer);
    // send to SFC
    downStream1.send(responsePacketBuffer, respHdr->len + sizeof(housekeeping_hdr_t) + 1 );  
    currentPacketCount++;
  }  
}
