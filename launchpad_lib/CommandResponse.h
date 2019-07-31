/*
 * CommandResponse.h
 * 
 * Declares a set of functions to act as responses to received commands and 
 * error protocols
 *
 */

#pragma once

#include "iProtocol.h"
#include <PacketSerial.h>
#include <Arduino.h>

/* If these commands are received */
void whatToDoIfPingPong(uint8_t * data);

void whatToDoIfISR(uint8_t * data);

void whatToDoIfMap(housekeeping_hdr_t * hdr_out, uint8_t (&downDevs)[7][254], uint8_t numDev);

void whatToDoIfSetPriority(housekeeping_prio_t * hdr_prio, housekeeping_hdr_t * hdr_out, uint8_t * comPriorList);
														
int whatToDoIfHeaterControl(uint8_t * data, uint8_t len);

int whatToDoIfTestHeaterControl(uint8_t* data, uint8_t len);

//void whatToDoIfTestMode(uint16_t * numTestPackets_p, uint8_t * data);

bool checkThatPriority(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out, uint8_t numSends);
												  
/* If the criteria for these errors is fulfilled  */
void error_badDest(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out, housekeeping_err_t * hdr_err);
void error_badCommand(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out, housekeeping_err_t * hdr_err);
void error_badLength(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out, housekeeping_err_t * hdr_err);
void error_badArgs(housekeeping_hdr_t * hdr_in, housekeeping_hdr_t * hdr_out, housekeeping_err_t * hdr_err);	

/* Matches on-board data with outgoing packets */                          				
void matchData(housekeeping_hdr_t * hdr_out);
