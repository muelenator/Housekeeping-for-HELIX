#pragma once

#include "iProtocol.h"
#include <Arduino.h>

/* Declare functions */
void whatToDoIfPingPong(housekeeping_hdr_t * hdr_out);
void whatToDoIfFSR(housekeeping_hdr_t * hdr_out);
void error_baddest(housekeeping_hdr_t * hdr_in, housekeeping_err_t * hdr_err);
void error_badargs(housekeeping_hdr_t * hdr_in, housekeeping_err_t * hdr_err);
void whatToDoIfMap(housekeeping_hdr_t * hdr_out, uint8_t * downDevs, uint8_t numDev);
void matchData(housekeeping_hdr_t * hdr_out);
