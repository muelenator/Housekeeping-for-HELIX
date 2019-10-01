/*
 * MagnetHSK_protocol.h
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
typedef enum MagnetHSK_cmd {
  // 2-248 are board-specific: these are test commands
  // 2-8 are resistance readings
  eResistanceCh3 = 2,
  eResistanceCh6 = 3,
  eResistanceCh9 = 4,
  eResistanceCh12 = 5,
  eResistanceCh16 = 6,
  eResistanceCh20 = 7,
  // 9-15 are temperature readings
  eTempCh3 = 8,
  eTempCh6 = 9,
  eTempCh9 = 10,
  eTempCh12 = 11,
  eTempCh16 = 12,
  eTempCh20 = 13,
  // 16-18 are flow readings
  eFlow1 = 14,
  eFlow2 = 15,
  // 16-25 are read temperature probes
  eTempProbe1 = 16,
  eTempProbe2 = 17,
  eTempProbe3 = 18,
  eTempProbe4 = 19,
  eTempProbe5 = 20,
  eTempProbe6 = 21,
  eTempProbe7 = 22,
  eTempProbe8 = 23,
  eTempProbe9 = 24,
  eTempProbe10 = 25,
  // 26 is read pressure
  ePressure = 26,

} MagnetHSK_cmd;
