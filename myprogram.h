#pragma once

#include "iProtocol.h"

/* Declare functions */
void startUp();
void setupMyPacket(housekeeping_hdr_t * hdr_out);
void whatToDoIfPingPong(housekeeping_hdr_t * hdr_in);
void whatToDoIfFSR(housekeeping_hdr_t * hdr_in);
