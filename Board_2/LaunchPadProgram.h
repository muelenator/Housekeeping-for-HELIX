#pragma once

#include "iProtocol.h"
#include <Arduino.h>

/* Declare functions */
void whatToDoIfPingPong(housekeeping_hdr_t * hdr_out);
void whatToDoIfFSR(housekeeping_hdr_t * hdr_out);
void matchData(housekeeping_hdr_t * hdr_out);
