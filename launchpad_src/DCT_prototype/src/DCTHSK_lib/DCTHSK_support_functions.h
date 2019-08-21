/*
 * CommandResponse.h
 * 
 * Declares a set of functions to act as responses to received commands and 
 * error protocols
 *
 */

#pragma once

#include <PacketSerial.h>
#include <Arduino.h>

/* If these commands are received */
void whatToDoIfISR(uint8_t * data);
							
int whatToDoIfHeaterControl(uint8_t * data, uint8_t len);

int whatToDoIfTestHeaterControl(uint8_t* data, uint8_t len, uint8_t * respData);

int whatToDoIfThermistors(uint8_t* respData);
