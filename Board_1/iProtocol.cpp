#include "iProtocol.h"

/*******************************************************************************
* Functions
*******************************************************************************/
/* Computes the checksum of a message
 * Parameters are the range of an array, lowest value is closed, highest is open*/
uint8_t checkDat = 0;
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

/* Returns true if two checksum values are the same */
bool checkMySum(uint8_t & checkI, const uint8_t & checkO)
{
	if (checkI != checkO) return false;

	else return true;
}

/* Find address in an array of addresses */
uint8_t * findMe(uint8_t * first, uint8_t * last, uint8_t address)
{
	while (first!=last) {
    if (*first==address) return first;
    ++first;
	}
  return last;
}
