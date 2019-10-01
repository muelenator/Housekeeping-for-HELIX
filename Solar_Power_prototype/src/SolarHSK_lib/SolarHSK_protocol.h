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


/*******************************************************************************
 * Typedef enums
 *******************************************************************************/

/* Command definitions */
typedef enum SolarHSK_cmd {
  // 2-248 are board-specific: these are test commands
	eIntSensorRead = 2,
        ePacketCount = 3,
        eVoltageCurrentInput = 4,
        eVoltageCurrentOutput = 5,
        ePanelTemp = 6,
        eBoardTemp = 7,
        eInductorTemp = 8,

} SolarHSK_cmd;
