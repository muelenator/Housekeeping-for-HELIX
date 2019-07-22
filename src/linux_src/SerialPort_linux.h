/* 
 * SerialPort_linux.h
 *
 * Library for opening, reading, and writing to a serial port in COBS encoding 
 * on Linux.
 *
 */
#pragma once

#include "LinuxLib.h"

#include <cstdint>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

/* Max data length is: 
 *	--4 header bytes
 *	-- + 255 data bytes
 *  -- + 1 CRC (checksum) byte
 * For max packet size, 
 *	-- +1 COBS overhead
 *	-- +1 COBS packet marker
 */
#define MAX_PACKET_LENGTH (4 + 255 + 1) + 2
/* define WAIT_TIME for time to wait after connecting to board */
#define WAIT_TIME 2000

class SerialPort
{	
public:
    SerialPort(const char *portName);
    ~SerialPort();

	/* Define functions that SerialPort will use */
	int readSerialPort(char *buffer);
	bool writeSerialPort(char *buffer, unsigned int buf_size);
    bool isConnected();
    
    /* typdefs for On-package-received function */
	
private:
    int	 handler;
    bool connected;
    
	
};

