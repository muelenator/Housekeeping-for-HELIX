/* 
 * iProtocol.cpp
 *
 * Defines the interface protocol for cross device communication.
 *
 * Housekeeping packet consists of header
 * 0-255 payload bytes
 * 1 byte CRCS (or checksum)
 *
 */

/*****************************************************************************
 * Defines
 ****************************************************************************/
#include "iProtocol.h"

/* Keeps running count of checksum mod 255. Used in computeMySum() */
uint8_t checkDat = 0;

/*******************************************************************************
* Functions
*******************************************************************************/
/* Function flow:
 * --Takes in the location of the first and last bytes of a data packet
 * --Sums all bytes, then returns that sum mod 255
 * --NOTE: The last byte is the checksum which was computed by the sender
 *		--This byte is ignored in the calculation
 *		--i.e. first byte is closed, last byte is open 
 *
 * Function Params:
 * first:		The location of the first byte in the data packet
 * last:		The location of the last byte in the data packet	
 *				--This is passed by reference, need to fix for consistency
 *
 * Function variables:
 * checkDat:	The running total of summed bytes in the packet
 *
 */

uint8_t computeMySum(const uint8_t * first, const uint8_t & last)
{
	checkDat = 0;
	while (first!= &last)	
	{
		checkDat += *first;
		++first;
	}
	return checkDat;
}

/* Function flow:
 * --Returns true if the sender checksum (CheckI) matches the computed checksum
 *	 (checkO)
 *
 * Function params:
 * checkI:		Checksum computed by the sender
 * checkO:		Checksum computed by this device from the sender's packet
 * 
 */
bool checkMySum(uint8_t & checkI, const uint8_t & checkO)
{
	if (checkI != checkO) return false;

	else return true;
}

/* Function flow:
 * --Checks if a device address is inside a given array of addresses
 * --If the device is known (within the list), function returns its location
 *	 in the list
 * --If the device is not known, returns the location of the end of the list
 *
 * Function params:
 * first:		The location of the address known to this device
 * last:		The location of the last address known to this device
 * address:		The name of the address we are trying to find
 * 
 */
uint8_t * findMe(uint8_t * first, uint8_t * last, uint8_t address)
{
	while (first!=last) 
	{
		if (*first==address) return first;
		++first;
	}
	return last;
}
