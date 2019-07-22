/*
 * LinuxLib.h
 *
 * Provides helper functions for SerialPort_linux. Sets up the serial 
 * connection and preps the serial bus.
 *
 * Function declarations.
 * 
 */

#pragma once

#include <iostream>

#include <cstdint>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

/* Set up the serial port for raw data communication */
int set_interface_attribs (int portName, int speed, int parity);

/* Change the minimum bytes & timeout for the connection on the fly */
void set_mincount (int portName, int should_block);
