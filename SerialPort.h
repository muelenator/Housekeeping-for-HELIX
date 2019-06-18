/* 
 * PS_SerialPort.h
 *
 * Header file declares the SerialPort class
 *
 */

#ifndef SERIALPORT_H
#define SERIALPORT_H

/* define MAX_DATA_LENGTH sizeof(housekeeping_hdr_t)+255*sizeof(uint8_t) + 1 */
#define MAX_DATA_LENGTH (4 + 255 + 1)
#define ARDUINO_WAIT_TIME 2000

#include <cstdint>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

class SerialPort
{	
public:
    SerialPort(const char *portName);
    ~SerialPort();

	/* Define functions that SerialPort will use */
    int update(uint8_t *buffer);
    bool send(uint8_t *buffer, size_t buf_size);
    bool isConnected();
    
    /* typdefs for On-package-received function */
    typedef void (*PacketHandlerFunction)(const uint8_t * buffer,
										  size_t size);										  
	typedef void (*PacketHandlerFunctionWithSender)(const void * sender,
													const uint8_t * buffer,
													size_t size);
	
	/* Functions for setting the packet handler	*/											
	void setPacketHandler(PacketHandlerFunction PacketReceivedFunction);
	void setPacketHandler(PacketHandlerFunctionWithSender PacketReceivedFunctionWithSender);
	
private:
    int	 handler;
    bool connected;
    
    /* COBS helpers for receiving an unknown packet */
	uint8_t _receiveBuffer[MAX_DATA_LENGTH] = {0};
	size_t _receiveBufferIndex = 0;
	/* On-packet-received function initialization */
	PacketHandlerFunction _PacketReceivedFunction = 0;
	PacketHandlerFunctionWithSender _PacketReceivedFunctionWithSender = 0;
	
};

#endif // SERIALPORT_H
