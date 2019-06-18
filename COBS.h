/* 
 * COBS.h
 *
 * Header file defines the COBS class
 *
 */

#ifndef COBS_h
#define COBS_h

#define PACKETMARKER 48

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>

class COBS
{
public:
	
	COBS();
	
    static size_t encode(const uint8_t* buffer,
                         size_t size,
                         uint8_t* encodedBuffer);
                         
    static size_t decode(const uint8_t* encodedBuffer,
                         size_t size,
                         uint8_t* decodedBuffer);
                         
    static size_t getEncodedBufferSize(size_t unencodedBufferSize);
};

#endif // COBS_H
