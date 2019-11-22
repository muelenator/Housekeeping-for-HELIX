/*
 * SolarHSK_protocol.cpp
 *
 * Declares the interface protocol for cross device communication.
 *
 * Housekeeping packet consists of header
 * 0-255 payload bytes
 * 1 byte CRCS (or checksum)
 */

/*****************************************************************************
 * Defines
 ****************************************************************************/
#pragma once
#include <stdint.h>

#define PCF8574_I2C_WRITE_ADDRESS 0x40
#define PCF8574_I2C_READ_ADDRESS 0x41
/*******************************************************************************
 * Typedef enums
 *******************************************************************************/

/* Command definitions */
typedef enum SolarHSK_cmd {
  // 2-248 are board-specific: these are test commands
	// eIntSensorRead = 2,
	
    ePower1 = 3,
	ePower2 = 4,
	
	eSense1 = 9,
	eSense2 = 10,
	
	eCurrent1 = 15,
	eCurrent2 = 16,
	
	eGPIO1 = 21,
	eGPIO2 = 22,
	eGPIO3 = 23,
	eGPIO4 = 24,
	
} SolarHSK_cmd;
