/*
 * MagnetHSK_prototype.ino
 *
 * Initiates serial ports & follows HSK protocol for command responses and error
 * reporting. This program can be used on other devices by changing the device
 * address (myID) and the downStream serial connection (direct line to the SFC)
 *
 * CONSTANTS:
 * --PACKETMARKER is defined in Cobs_encoding.h
 * --MAX_PACKET_LENGTH is defined in PacketSerial
 * --NUM_LOCAL_CONTROLS is defined here
 * --FIRST_LOCAL_COMMAND is defined here
 * --TEST_MODE_PERIOD is defined here
 * --BAUD rates are defined here
 *
 */

/* Everyone uses these */
#include <Core_protocol.h>
#include <PacketSerial.h>
#include <driverlib/sysctl.h>

/* These are device specific */
#include "src/MagnetHSK_lib/MagnetHSK_protocol.h"

#include "src/MagnetHSK_lib/PressureAndFlow.h"
#include "src/MagnetHSK_lib/configFunctions.h"
#include "src/MagnetHSK_lib/supportFunctions.h"
#include "src/MagnetHSK_lib/configConstants.h"
#include "src/MagnetHSK_lib/TempSensors.h"
///////////////////////////////////////
#define DOWNBAUD 115200
#define UPBAUD 9600
#define TEST_MODE_PERIOD 100      // period in milliseconds between testmode packets being sent
#define FIRST_LOCAL_COMMAND 2     // value of hdr->cmd that is the first command local to the board
#define NUM_LOCAL_CONTROLS 26 + 6 // how many commands total are local to the board

/* Declare instances of PacketSerial to set up the serial lines */
PacketSerial downStream1; // mainHSK
PacketSerial upStream1;  // StackFlow
PacketSerial upStream2;  // ShieldFlow

/* Name of this device */
housekeeping_id myID = eMagnetHsk;


/* Outgoing buffer, for up or downstream. Only gets used once a complete packet
 * is received -- a command or forward is executed before anything else happens,
 * so there shouldn't be any over-writing here. */
uint8_t outgoingPacket[MAX_PACKET_LENGTH]={0};

/* Use pointers for all device's housekeeping headers and the autopriorityperiods*/
housekeeping_hdr_t * hdr_in;     housekeeping_hdr_t * hdr_out;
housekeeping_err_t * hdr_err;   housekeeping_prio_t * hdr_prio;
/* Memory buffers for housekeeping system functions */
uint8_t numDevices = 0;           // Keep track of how many devices are upstream
uint8_t commandPriority[NUM_LOCAL_CONTROLS] = {0};     // Each command's priority takes up one byte
PacketSerial *serialDevices[2] = {&upStream1, &upStream2};
                      // Pointer to an address's serial port
uint8_t addressList[7][254] = {0}; // List of all upstream devices

/* Utility variables for internal use */
uint8_t checkin;    // Used for comparing checksum values
size_t hdr_size = sizeof(housekeeping_hdr_t)/sizeof(hdr_out->src); // size of the header
uint8_t numSends = 0; // Used to keep track of number of priority commands executed
int bus = 0;
uint16_t currentPacketCount=0;
unsigned long timelastpacket;
//static_assert(sizeof(float) == 4);

uint8_t localControlPriorities[NUM_LOCAL_CONTROLS] = {0}; // Priority settings

const int thermalPin = 38;

/*******************************************************************************
 * Setup
 *******************************************************************************/
void setup()
{
  Serial3.begin(DOWNBAUD);
  downStream1.setStream(&Serial3);
  downStream1.setPacketHandler(&checkHdr);
  //	upStream1.setStream(&Serial);
  //	upStream1.setPacketHandler(&checkHdr);

//  Serial.begin(UPBAUD);
//  upStream1.setStream(&Serial);
//  upStream1.setPacketHandler(&checkHdr);

//  Serial4.begin(UPBAUD);
//  upStream2.setStream(&Serial4);
//  upStream2.setPacketHandler(&checkHdr);

  pinMode(7, INPUT);
  // pinMode(6,OUTPUT);
  // digitalWrite(6,LOW);
  initRS232();
  Initialize_TM4C123();
  configure_channels();
  configure_memory_table();
  configure_global_parameters();
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
}

/*******************************************************************************
 * Packet handling functions
 *******************************************************************************/
void checkHdr(const void *sender, const uint8_t *buffer, size_t len) {
  // Default header & error data values
  hdr_out->src = myID;          // Source of data packet
  hdr_in = (housekeeping_hdr_t *)buffer;
  hdr_prio = (housekeeping_prio_t *) (buffer + hdr_size);
    // If an error occurs at this device from a message
  if (hdr_in->dst == eBroadcast || hdr_in->dst==myID) hdr_err->dst = myID;
  else hdr_err->dst = hdr_in->dst;
  // If the checksum didn't match, throw a bad args error
  // Check for data corruption
  if (!(verifyChecksum((uint8_t *) buffer))) {
      //error_badArgs(hdr_in, hdr_out, hdr_err);  
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
// forward downstream to the SFC
void forwardDown(const uint8_t * buffer, size_t len, const void * sender) {
  downStream1.send(buffer, len);
  checkDownBoundDst(sender);
  currentPacketCount++;
}
// forward upstream away from the SFC
void forwardUp(const uint8_t * buffer, size_t len) {
  bus = checkUpBoundDst();
  if (bus) serialDevices[bus]->send(buffer,len);
}

/* checkDownBoundDst Function flow:
 * --Checks to see if the downstream device that sent the message is known
 *    --If not, add it to the list of known devices
 *    --If yes, just carry on
 * --Executed every time a packet is received from downStream
 * 
 * Function params:
 * sender:    PacketSerial instance (serial line) where the message was received
 * 
 */
void checkDownBoundDst(const void * sender) {
  for (int i=0; i<7; i++) {
    if (serialDevices[i] == (PacketSerial *) sender) {
      if (addressList[i][hdr_in->src] == 0) {
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
  /* If device wasn't found when checking upbound addresses, throw an error */
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
 * Function params:
 * sender:    PacketSerial instance which triggered the error protocol
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
  buildError(hdr_err, hdr_out, hdr_in, EBADLEN);
  fillChecksum((uint8_t *) outgoingPacket);
  downStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
  currentPacketCount++;
}

// Function for building the error packets to send back when an error is found (see the Core_Protocol.h for the defs of the errors and the error typdefs).
void buildError(housekeeping_err_t *err, housekeeping_hdr_t *respHdr, housekeeping_hdr_t * hdr, int error){
  respHdr->cmd = eError;
  respHdr->len = 4;
  err->src = hdr->src;
  err->dst = hdr->dst;
  err->cmd = hdr->cmd;
  err->error = error;
}
/*******************************************************************************
 * END OF Packet handling functions
 *******************************************************************************/

// function for when a "SetPriority" command is received by this device, adding that commands priority value to the array/list
void setCommandPriority(housekeeping_prio_t * prio, uint8_t * respData, uint8_t len) {
//  housekeeping_prio_t * set_prio = (housekeeping_prio_t *) prio;
  commandPriority[prio->command-FIRST_LOCAL_COMMAND] = (uint8_t) prio->prio_type;
  memcpy(respData, (uint8_t*)prio, len);
}

// sending priority command function
// probably can be cleaned up
// Note: SendAll is 253 and SendLow is 250 so we made SendLow-> int priority=1 for checking the device's list of command's priorities.
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

// Fn to handle a local command write.
// This gets called when a local command is received
// with data (len != 0).
int handleLocalWrite(uint8_t localCommand, uint8_t *data, uint8_t len, uint8_t * respData) {
  int retval = 0;
  switch (localCommand) {
  case eSetPriority: {
    setCommandPriority((housekeeping_prio_t *)data,respData,len);
    retval=len;
    break;
  }
  case eResistanceCh3: {
    retval = EBADLEN;
    break;
  }
  case eResistanceCh6: {
    retval = EBADLEN;
    break;
  }
  case eResistanceCh9: {
    retval = EBADLEN;
    break;
  }
  case eResistanceCh12: {
    retval = EBADLEN;
    break;
  }
  case eResistanceCh16: {
    retval = EBADLEN;
    break;
  }
  case eResistanceCh20: {
    retval = EBADLEN;
    break;
  }
  case eTempCh3: {
    retval = EBADLEN;
    break;
  }
  case eTempCh6: {
    retval = EBADLEN;
    break;
  }
  case eTempCh9: {
    retval = EBADLEN;
    break;
  }
  case eTempCh12: {
    retval = EBADLEN;
    break;
  }
  case eTempCh16: {
    retval = EBADLEN;
    break;
  }
  case eTempCh20: {
    retval = EBADLEN;
    break;
  }
  case eFlow1: {
    retval = EBADLEN;
    break;
  }
  case eFlow2: {
    retval = EBADLEN;
    break;
  }
  case eTempProbe1: {
    retval = EBADLEN;
    break;
  }
  case eTempProbe2: {
    retval = EBADLEN;
    break;
  }
  case eTempProbe3: {
    retval = EBADLEN;
    break;
  }
  case eTempProbe4: {
    retval = EBADLEN;
    break;
  }
  case eTempProbe5: {
    retval = EBADLEN;
    break;
  }
  case eTempProbe6: {
    retval = EBADLEN;
    break;
  }
  case eTempProbe7: {
    retval = EBADLEN;
    break;
  }
  case eTempProbe8: {
    retval = EBADLEN;
    break;
  }
  case eTempProbe9: {
    retval = EBADLEN;
    break;
  }
  case eTempProbe10: {
    retval = EBADLEN;
    break;
  }
  case ePressure: {
    retval = EBADLEN;
    break;
  }
  case eReset: {
    retval = EBADLEN;
    break;
  }
  default:
    retval = EBADLEN;
    break;
  }
  return retval;
}

// Fn to handle a local command read.
// This gets called when a local command is received
// with no data (len == 0)
// buffer contains the pointer to where the data
// will be written.
// int returns the number of bytes that were copied into
// the buffer, or EBADCOMMAND if there's no command there
int handleLocalRead(uint8_t localCommand, uint8_t *buffer) {
  int retval = 0;
  switch (localCommand) {
  case ePingPong:
    retval=0;
    break;
  case eSetPriority:
    retval = EBADLEN;
    break;
  /* Resistance measurements */
  case eResistanceCh3: {
    float ResRead = returnResistance(CHIP_SELECT, 3);
    memcpy(buffer, (uint8_t *)&ResRead, sizeof(ResRead));
    retval = (int) sizeof(ResRead);
    break;
  }
  case eResistanceCh6: {
    float ResRead = returnResistance(CHIP_SELECT, 6);
    memcpy(buffer, (uint8_t *)&ResRead, sizeof(ResRead));
    retval = (int) sizeof(ResRead);
    break;
  }
  case eResistanceCh9: {
    float ResRead = returnResistance(CHIP_SELECT, 9);
    memcpy(buffer, (uint8_t *)&ResRead, sizeof(ResRead));
    retval = (int) sizeof(ResRead);
    break;
  }
  case eResistanceCh12: {
    float ResRead = returnResistance(CHIP_SELECT, 12);
    memcpy(buffer, (uint8_t *)&ResRead, sizeof(ResRead));
    retval = (int) sizeof(ResRead);
    break;
  }
  case eResistanceCh16: {
    float ResRead = returnResistance(CHIP_SELECT, 16);
    memcpy(buffer, (uint8_t *)&ResRead, sizeof(ResRead));
    retval = (int) sizeof(ResRead);
    break;
  }
  case eResistanceCh20: {
    float ResRead = returnResistance(CHIP_SELECT, 20);
    memcpy(buffer, (uint8_t *)&ResRead, sizeof(ResRead));
    retval = (int) sizeof(ResRead);
    break;
  }
  /* Temperature measurements */
  case eTempCh3: {
    float TempRead = returnTemperature(CHIP_SELECT, 3);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = sizeof(TempRead);
    break;
  }
  case eTempCh6: {
    float TempRead = returnTemperature(CHIP_SELECT, 6);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = sizeof(TempRead);
    break;
  }
  case eTempCh9: {
    float TempRead = returnTemperature(CHIP_SELECT, 9);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  case eTempCh12: {
    float TempRead = returnTemperature(CHIP_SELECT, 12);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  case eTempCh16: {
    float TempRead = returnTemperature(CHIP_SELECT, 16);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  case eTempCh20: {
    float TempRead = returnTemperature(CHIP_SELECT, 20);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  /* Flow readings */
  case eFlow1: {
     double gasdata[4];
    char gastype[100];
    char errorcode[100];
    bool FlowRead = readFlow(1, gasdata, gastype, errorcode);
    uint8_t flowread_ft;
    if(FlowRead){
      flowread_ft = 0;
      memcpy(buffer, (uint8_t *)&gasdata, sizeof(gasdata));
      memcpy(buffer + sizeof(gasdata), (uint8_t *)&gastype, sizeof(gastype));
      memcpy(buffer + sizeof(gasdata) + sizeof(gastype), &flowread_ft, sizeof(flowread_ft));
      retval = (sizeof(gasdata) + sizeof(gastype)+sizeof(flowread_ft));    
    }
    else {
      flowread_ft= 1;
      memcpy(buffer, (uint8_t *)&errorcode, sizeof(errorcode));
      memcpy(buffer + sizeof(errorcode), &flowread_ft, sizeof(flowread_ft));
      retval = (sizeof(errorcode)+sizeof(flowread_ft));
    }
    break;
  }
  case eFlow2: {
    double gasdata[4];
    char gastype[100];
    char errorcode[100];
    bool FlowRead = readFlow(2, gasdata, gastype, errorcode);
    uint8_t flowread_ft;
    if(FlowRead){
      flowread_ft = 0;
      memcpy(buffer, (uint8_t *)&gasdata, sizeof(gasdata));
      memcpy(buffer + sizeof(gasdata), (uint8_t *)&gastype, sizeof(gastype));
      memcpy(buffer + sizeof(gasdata) + sizeof(gastype), &flowread_ft, sizeof(flowread_ft));
      retval = (sizeof(gasdata) + sizeof(gastype)+sizeof(flowread_ft));    
    }
    else {
      flowread_ft= 1;
      memcpy(buffer, (uint8_t *)&errorcode, sizeof(errorcode));
      memcpy(buffer + sizeof(errorcode), &flowread_ft, sizeof(flowread_ft));
      retval = (sizeof(errorcode)+sizeof(flowread_ft));
    }
    break;
  }
  /* Temperature probes */
  case eTempProbe1: {
    float TempRead = tempSensorVal(1, thermalPin);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  case eTempProbe2: {
    float TempRead = tempSensorVal(2, thermalPin);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  case eTempProbe3: {
    float TempRead = tempSensorVal(3, thermalPin);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  case eTempProbe4: {
    float TempRead = tempSensorVal(4, thermalPin);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  case eTempProbe5: {
    float TempRead = tempSensorVal(5, thermalPin);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  case eTempProbe6: {
    float TempRead = tempSensorVal(6, thermalPin);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  case eTempProbe7: {
    float TempRead = tempSensorVal(7, thermalPin);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  case eTempProbe8: {
    float TempRead = tempSensorVal(8, thermalPin);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  case eTempProbe9: {
    float TempRead = tempSensorVal(9, thermalPin);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  case eTempProbe10: {
    float TempRead = tempSensorVal(10, thermalPin);
    memcpy(buffer, (uint8_t *)&TempRead, sizeof(TempRead));
    retval = (int) sizeof(TempRead);
    break;
  }
  /* Pressure Readings */
  case ePressure: {
    double pressureValue;
    double temperature;
    char typeOfPressure;
    char errorcode[100];
    bool pressure_bool = readPressureSensor(&pressureValue, &temperature, &typeOfPressure, errorcode);
    uint8_t pressure_ft;
    if(pressure_bool){
      pressure_ft=0;
      memcpy(buffer, (uint8_t *)&pressureValue, sizeof(pressureValue));
      memcpy(buffer + sizeof(pressureValue), (uint8_t *)&temperature, sizeof(temperature));
      memcpy(buffer + sizeof(pressureValue)+sizeof(temperature), (uint8_t *) &typeOfPressure, sizeof(typeOfPressure));
      retval = (sizeof(pressureValue) + sizeof(temperature) + sizeof(typeOfPressure) + sizeof(pressure_ft));
    }
    else {
      pressure_ft= 1;
      memcpy(buffer, (uint8_t *)&errorcode, sizeof(errorcode));
      memcpy(buffer + sizeof(errorcode), &pressure_ft, sizeof(pressure_ft));
      retval = (sizeof(errorcode)+sizeof(pressure_ft));
    }
    break;
  }
  
  case eReset: {
    SysCtlReset();
    return 0;
  }
  default:
    retval = EBADCOMMAND;
  }
  return retval;
}

// Function to call first when localcommand sent. 
// Store the "result" as retval (which is the bytes read or written, hopefully)
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

/******************************************************************************
 * Device specific functions
 *****************************************************************************/
// extra space for functions related to this device.
