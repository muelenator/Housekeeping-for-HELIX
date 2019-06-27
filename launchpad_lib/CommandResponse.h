#pragma once

#include "iProtocol.h"
#include <Arduino.h>

/* Declare functions */
void whatToDoIfPingPong(housekeeping_hdr_t * hdr_out);
void whatToDoIfFSR(housekeeping_hdr_t * hdr_out);
void whatToDoIfMap(housekeeping_hdr_t * hdr_out, uint8_t * downDevs, uint8_t numDev);
void whatToDoIfSetPriority(housekeeping_prio_t * hdr_prio, housekeeping_hdr_t * hdr_out, 
														uint8_t * comPriorList);
bool checkThatPriority(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out,
												    uint8_t numSends);
												  

void error_badDest(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out,
												housekeeping_err_t * hdr_err);
void error_badCommand(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out,
												   housekeeping_err_t * hdr_err);
void error_badLength(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out,
												  housekeeping_err_t * hdr_err);
void error_badArgs(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out,
												housekeeping_err_t * hdr_err);	
											

void matchData(housekeeping_hdr_t * hdr_out);
