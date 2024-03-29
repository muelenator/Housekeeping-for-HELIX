/*
 * DCTHSK_protocol.cpp
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

#define MIN_PERIOD      100

/*******************************************************************************
 * Typedef enums
 *******************************************************************************/

/* Command definitions */
typedef enum DCTHSK_cmd {
  // 2-248 are board-specific: these are test commands
	eIntSensorRead = 2,
	eHeaterControl = 3,
	eTestHeaterControl = 4,
        eThermistors = 5,
        ePacketCount = 6,
} DCTHSK_cmd;
