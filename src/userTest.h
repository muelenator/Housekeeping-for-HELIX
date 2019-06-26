/* 
 * PingAndRead.h
 *
 * General template functions for a specific use case of the main.cpp program.
 * --Program prompts the user for an intended destination and command, then sends 
 * 	 out a COBS encoded data packet to that destination.
 *
 */


#pragma once

#include "../src/iProtocol.h"
#include <iostream>

/* Startup function for user interface */
void startUp();

/* Setting the interface protocol for this device */
void setupMyPacket(housekeeping_hdr_t * hdr_out);

/* This device's response to ping pong command */
void whatToDoIfPingPong(housekeeping_hdr_t * hdr_in);

/* This device's response to fake sensor read command */
void whatToDoIfFSR(housekeeping_hdr_t * hdr_in);


void whatToDoIfError(housekeeping_err_t * hdr_err, uint8_t * errorsReceived, uint8_t & numError);
					
void whatToDoIfMap(housekeeping_hdr_t * hdr_in);
					
void resetAll(housekeeping_hdr_t * hdr_out);



/* Puts the outgoing data inside the outgoing packet */
void matchData(housekeeping_hdr_t * hdr_out);