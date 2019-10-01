/*
 * MainHSK_protocol.cpp
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
typedef enum MainHSK_cmd {
  // 2-248 are board-specific: these are test commands
	eIntSensorRead = 2,
	ePacketCount = 3,
	eMapDevices = 4,
	eHeaterControl = 5,
	eTestHeaterControl = 6,
        eAutoPriorityPeriod = 7,
} MainHSK_cmd;

/* Autopriority period */
typedef struct autoPriorityPeriods {
  uint16_t lowPriorityPeriod; // auto low pri. period, in ms (0 disables, 100-655)
  uint16_t medPriorityPeriod; // as above, for med pri.
  uint16_t highPriorityPeriod;  // as above, for hi pri.
} autoPriorityPeriods_t;
