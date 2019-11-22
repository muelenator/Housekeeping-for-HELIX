/*
 * SolarHSK_prototype.ino
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
#include "src/SolarHSK_lib/LTC2992.h"
#include "src/SolarHSK_lib/SolarHSK_protocol.h"
#include "src/SolarHSK_lib/DCT_I2C_support_functions.h"
#define DOWNBAUD 115200 // Baudrate to the SFC
#define UPBAUD 9600    // Baudrate to upsteam devices
#define TEST_MODE_PERIOD 100 // period in milliseconds between testmode packets being sent
#define FIRST_LOCAL_COMMAND 2 // value of hdr->cmd that is the first command local to the board
#define NUM_LOCAL_CONTROLS 7 // how many commands total are local to the board
int8_t LTC2992_mode = 0;
const int port_0 = 0;
uint8_t localControlPriorities[NUM_LOCAL_CONTROLS] = {0}; // Priority settings

/* Declare instances of PacketSerial to set up the serial lines */
PacketSerial downStream1;
/*******************************************************************************
* Defines
*******************************************************************************/
/* Name of this device */
housekeeping_id myID = eSolarHSK;

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

//  
//  Serial.begin(UPBAUD);
  
  Serial.begin(DOWNBAUD);
  downStream1.setStream(&Serial);
  downStream1.setPacketHandler(&checkHdr);

// Connect to I2C port of MPPT which is LTC2992 (Orthogonal Systems)
  // one way is the functions in src/SolarHSK_lib/I2C.cpp ?
  InitI2C0();  
  uint32_t cont = I2CSendNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_CTRLA_REG, LTC2992_mode);

  if (cont != 0) {
	  buildError(cont);
      fillChecksum((uint8_t *) outgoingPacket);
      downStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
	  SysCtlReset();
  }
	  
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
}

void checkHdr(const void *sender, const uint8_t *buffer, size_t len)
{
  hdr_in = (housekeeping_hdr_t *)buffer;
  /* Check if the message is for this device. If so, check & execute command */
  if (hdr_in->dst == myID || hdr_in->dst == eBroadcast)
  {
    /* Check for data corruption */
    if (verifyChecksum((uint8_t *)buffer))
    {
      /* Check for bad length	*/
      if (hdr_in->len != len - 4 - 1)
      {
        badPacketReceived((PacketSerial *)sender);
        return;
      }

      /* If a send priority command is received */
      if ((int)hdr_in->cmd <= 253 && (int)hdr_in->cmd >= 250)
        handlePriority((hdr_in->cmd - 249) % 4); // passes the priority #
      /* Otherwise just execute the command */
      else
        handleLocalCommand(hdr_in, (uint8_t *)hdr_in + hdr_size);
    }

    /* If the checksum didn't match, throw a bad args error */
    else
    {
      buildError(EBADARGS);
      fillChecksum((uint8_t *)outgoingPacket);
      downStream1.send(outgoingPacket, hdr_size + 4 + 1);
    }
  }

  /* If the message wasn't meant for this device pass it along */
  else
  {
    buildError(EBADDEST);
    fillChecksum((uint8_t *)outgoingPacket);
    downStream1.send(outgoingPacket, hdr_size + 4 + 1);
  }
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
  hdr_in->src = eMainHsk;
  hdr_out->src = myID;
  buildError(EBADLEN);
  fillChecksum((uint8_t *) outgoingPacket);
  downStream1.send(outgoingPacket, hdr_size + hdr_out->len + 1);
  currentPacketCount++;
}
// got a priority request from destination dst
void handlePriority(uint8_t priority)
{
  for (int i = FIRST_LOCAL_COMMAND; i < NUM_LOCAL_CONTROLS; i++)
  {
    hdr_in->cmd = i;
    if (localControlPriorities[i] == priority || !priority)
      handleLocalCommand(hdr_in, (uint8_t *)hdr_in + hdr_size);
  }
}

void setCommandPriority(housekeeping_prio_t * prio, uint8_t * respData, uint8_t len) {
//  housekeeping_prio_t * set_prio = (housekeeping_prio_t *) prio;
  commandPriority[prio->command-FIRST_LOCAL_COMMAND] = (uint8_t) prio->prio_type;
  memcpy(respData, (uint8_t*)prio, len);
}

int handleLocalWrite(uint8_t localCommand, uint8_t *data, uint8_t len)
{
  switch (localCommand)
  {	  
  case eSetPriority:
  {
    housekeeping_prio_t *in_prio = (housekeeping_prio_t *)data;
    housekeeping_prio_t *out_prio = (housekeeping_prio_t *)(outgoingPacket + hdr_size);
    localControlPriorities[in_prio->command] = in_prio->prio_type;

    memcpy(out_prio, (uint8_t *)in_prio, sizeof(housekeeping_prio_t));
    return sizeof(housekeeping_prio_t);
  }
  case eReset:
  {
    return EBADLEN;
  }
  default:
    return EBADCOMMAND;
  }
}


int handleLocalRead(uint8_t localCommand, uint8_t *buffer)
{
  switch (localCommand)
  {
  case ePingPong:
    return 0;
	
  case ePower1:
	uint32_t err; 
	err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 3, LTC2992_POWER1_MSB2_REG, buffer);
	err |= I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 3, LTC2992_MAX_POWER1_MSB2_REG, (buffer+4));
	err |= I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 3, LTC2992_MIN_POWER1_MSB2_REG, (buffer+8));
	if (!err) return 12;
  case ePower2:
	err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 3, LTC2992_POWER2_MSB2_REG, buffer);
	err |= I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 3, LTC2992_MAX_POWER2_MSB2_REG, (buffer+4));
	err |= I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 3, LTC2992_MIN_POWER2_MSB2_REG, (buffer+8));
	*buffer =  0x0FFFFFF & *buffer;
	if (!err) return 12;
	
  case eSense1:
	err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_SENSE1_MSB_REG, buffer);
	err |= I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MAX_SENSE1_MSB_REG, (buffer+2));
	err |= I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MIN_SENSE1_MSB_REG, (buffer+4));
	if (!err) return 6;
  case eSense2:
	err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_SENSE2_MSB_REG, buffer);
	err |= I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MAX_SENSE2_MSB_REG, (buffer+2));
	err |= I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MIN_SENSE2_MSB_REG, (buffer+4));
	if (!err) return 6;

  case eCurrent1:
	err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_DELTA_SENSE1_MSB_REG, buffer);
	err |= I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MAX_DELTA1_SENSE_MSB_REG, (buffer+2));
	err |= I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MIN_DELTA1_SENSE_MSB_REG, (buffer+3));
	if (!err) return 6;
  case eCurrent2:
	err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_GPIO1_MSB_REG, buffer);
    err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MAX_GPIO1_MSB_REG, (buffer+2));
    err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MIN_GPIO1_MSB_REG, (buffer+4));
	if (!err) return 6;
	
  case eGPIO1:
	err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_GPIO1_MSB_REG, buffer);
    err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MAX_GPIO1_MSB_REG, (buffer+2));
    err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MIN_GPIO1_MSB_REG, (buffer+4));
	if (!err) return 6;
  case eGPIO2:
	err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_GPIO2_MSB_REG, buffer);
    err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MAX_GPIO2_MSB_REG, (buffer+2));
    err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MIN_GPIO2_MSB_REG, (buffer+4));
	if (!err) return 6;
  case eGPIO3:
	err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_GPIO3_MSB_REG, buffer);
    err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MAX_GPIO3_MSB_REG, (buffer+2));
    err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MIN_GPIO3_MSB_REG, (buffer+4));
	if (!err) return 6;
  case eGPIO4:
	err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_GPIO4_MSB_REG, buffer);
    err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MAX_GPIO4_MSB_REG, (buffer+2));
    err = I2CReadNbytes(port_0, LTC2992_I2C_ADDRESS, 2, LTC2992_MIN_GPIO4_MSB_REG, (buffer+4));
	if (!err) return 6;		
	
  case eSetPriority:
    return EBADLEN;
  case eReset:
    SysCtlReset();
    return 0;
  default:
    return EBADCOMMAND;
  }
}

void buildError(int error)
{
  housekeeping_hdr_t *respHdr = (housekeeping_hdr_t *)outgoingPacket;
  housekeeping_err_t *err = (housekeeping_err_t *)(outgoingPacket + hdr_size);
  respHdr->dst = eSFC;
  respHdr->cmd = eError;
  respHdr->len = 4;
  err->src = hdr_in->src;
  err->dst = hdr_in->dst;
  err->cmd = hdr_in->cmd;
  err->error = error;
}

void handleLocalCommand(housekeeping_hdr_t *hdr, uint8_t *data)
{
  int retval;

  housekeeping_hdr_t *respHdr = (housekeeping_hdr_t *)outgoingPacket;
  uint8_t *respData = outgoingPacket + hdr_size;

  if (hdr->len) // local write
    retval = handleLocalWrite(hdr->cmd, data, hdr->len);
  else // local read. by definition these always go downstream.
    retval = handleLocalRead(hdr->cmd, respData);

  if (retval >= 0)
  {
    respHdr->src = myID;
    respHdr->dst = hdr->src;
    respHdr->cmd = hdr->cmd;
    respHdr->len = retval;
  }
  else
    buildError(retval);

  fillChecksum(outgoingPacket);
  downStream1.send(outgoingPacket, hdr_size + respHdr->len + 1);

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
    buildError(EBADLEN); 
    fillChecksum(responsePacketBuffer);
    // send to SFC
    downStream1.send(responsePacketBuffer, respHdr->len + sizeof(housekeeping_hdr_t) + 1 );  
    currentPacketCount++;
  }  
}
