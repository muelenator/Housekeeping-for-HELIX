/* InterfaceProtocol.h
 * 
 *
 * Housekeeping packet consists of header
 * 0-255 payload bytes
 * 1 byte CRCS (or checksum)
 */
 
#pragma once
#include <stdint.h>

/*******************************************************************************
* Typedefs
*******************************************************************************/
/* Src/dst definitions. 255 is illegal for source. */
typedef enum housekeeping_id 
{
	eSFC = 0,
	eMainHsk = 1,
	eMagnetHsk = 2,
	eDCTHsk = 3,
	eSPHsk1 = 4,
	eSPHsk2 = 5,
	// etc...
	eBroadcast = 255
} housekeeping_id;
 
/* Command definitions */
typedef enum housekeeping_cmd
{
	ePingPong = 0,
	eSetPriority = 1,
	eFakeSensorRead = 2,
	//2-249 are board-specific
	eSendLowPriority = 250,
	eSendMedPriority = 251,
	eSendHiPriority = 252,
	eSendAll = 253,
	eReset = 254,
	eError = 255
} housekeeping_cmd;

typedef struct housekeeping_hdr_t 
{
	uint8_t src;	// Source of packet
	uint8_t dst; 	// Destination of packet
	uint8_t cmd; 	// Command (or response) type
	uint8_t len; 	// Bytes to follow - 1	
} housekeeping_hdr_t;


/*******************************************************************************
* Functions
*******************************************************************************/
/* Computes the checksum of a message
 * Parameters are the range of an array, lowest value is closed, highest is open*/
uint8_t computeMySum(const uint8_t * first, const uint8_t & last);

/* Returns true if two checksum values are the same */
bool checkMySum(uint8_t & checkI, const uint8_t & checkO);

/* Find address in an array of addresses */
uint8_t * findMe(uint8_t * first, uint8_t * last, uint8_t address);


/*******************************************************************************
* Defines - global variables
*******************************************************************************/
extern housekeeping_hdr_t * hdr_in;		// pointer for loc of incoming data header
extern housekeeping_hdr_t * hdr_out;	// pointer for loc of outgoing header
