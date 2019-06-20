#include <PacketSerial.h>
#include <iProtocol.h>


/* Declare an instance of PacketSerial to set up the serial bus */
PacketSerial upStream1;

/*******************************************************************************
* Defines
*******************************************************************************/

/* Name this device */
housekeeping_id myID = eMagnetHsk;

/* Declarations to keep track of device list */
uint8_t downStreamDevices[254] = {0};	// Keep a list of downstream devices
uint8_t numDevices = 0;			// Keep track of how many devices are downstream

/* Create buffers for data */
uint8_t outgoingPacket [MAX_DATA_LENGTH]; 	// Buffer for outgoing packet

/* Guess: defining the variable here makes the linker connect where these
 * 		  variables are being used 	*/
housekeeping_hdr_t * hdr_out;
uint8_t data_in[4][255]= {"this is the MainHSK board, check the header to ensure", "this is the MagnetHSK board, check the header to ensure", "this is the DCTHSK board, check the header to ensure", "this is the SPHSK (whatevs that means) board, check the header to ensure"};

/* Checksum values */
uint8_t checkin;
uint8_t* ptr;
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

	/* Point to data in a way that it can be read as a header_hdr_t */
	hdr_out = (housekeeping_hdr_t *) outgoingPacket;

}

void loop()
{
  
  for(int i=0;i<4;i++){
     hdr_out->src=i+1; // set hdr_out types
     hdr_out->cmd = 2;
     hdr_out->dst=0;
     hdr_out->len=calc_length_data(data_in[i]);
     matchData(hdr_out,i); //call match function which fills outgoing packets data portion with data_in stuff
     // calculate length of packet 
     size_t len_packet=(size_t)hdr_out->len +4+1;
    /* Compute checksum */
     outgoingPacket[4+hdr_out->len] = computeMySum(outgoingPacket,outgoingPacket[4+hdr_out->len]);
	/* Continuously read in one byte at a time until a packet is received */
	   upStream1.send(outgoingPacket, len_packet);
     delay(3000);
   }
}

/*******************************************************************************
* Functions
*******************************************************************************/
/* To be executed when a packet is received */
void isItMe(const void * sender, const uint8_t * buffer, size_t len)
{
    /* Read in the header */
}

uint8_t calc_length_data(uint8_t *data){
    
    for(int i=0; i<255; i++){
      if(*(data)==0) return i;
      data++;
    }
}

/* Function flow:
* --Matches an array of data with the (global) outgoingPacket data slots
* --Requires outgoingData as a global uint8_tarray
 *
* Function params:
* hdr_out:             Pointer to the first byte of the incoming packet
*                            --As a housekeeping_hdr_t type, contains a src, dst, cmd, & len
*
* Function variables:
* ptr:                     Dummy pointer to where we want to put this byte in the outgoing
*                            packet. Gets iterated over
*
 */
void matchData(housekeeping_hdr_t * hdr_out, int j)
{

    ptr = (uint8_t *) hdr_out;
        for (int i = 0; i < hdr_out->len; i++){
            *(ptr+4+i) = data_in[j][i];
        }

}

 
