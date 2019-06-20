/* 
 * SerialPort_linux.cpp
 *
 * Defines SerialPort class for reading, writing, and opening a port on linux
 * for COBS encoded packets.
 *
 */

/*****************************************************************************
 * Defines
 ****************************************************************************/
#include "SerialPort_linux.h"
#include "../COBS.h"

using std::cout;
using std::endl;

/*****************************************************************************
 * Contructor/Destructor
 ****************************************************************************/
/* Function flow:
 * --SerialPort instance: Opens up a serial port with specific parameters.
 * --If the port the user chose in main.cpp cannot be opened, spit out an error
 *
 * Function params:
 * portName:		Name of our opened serial port in SerialPort_linux
 * 
 */
SerialPort::SerialPort(const char *portName)
{
    this->connected = false;

    this->handler = open (portName, O_RDWR | O_NOCTTY | O_SYNC); //)_NDELAY  
    
    if (this->handler < 0)
    {
	//	error_message ("error %d opening %s: %s", errno, portName, strerror (errno));
		return;
    }
    /* Setup serial port parameters in a DCB struct */
    else 
    {
   		set_interface_attribs (this->handler, B9600, 0); // set speed to 9,600 bps, 8n1 (no parity)
		set_mincount (this->handler, 0); 	// set no blocking  		
        this->connected = true;				// set status to 'connected'
        usleep(WAIT_TIME);					// sleep until the board wakes up
    }
}

/* Define the destructor to close the port */
SerialPort::~SerialPort()
{
    if (this->connected){
        this->connected = false;
        close(this->handler);		//Closing the serial port
    }
}

/*****************************************************************************
 * Functions
 ****************************************************************************/
 
/* Function Flow
 * --Sets the packet handler for this instance of SerialPort to a user-defined function 
 *
 * Function Params:
 * PacketReceivedFunction:				user-defined function location in memory
 * PacketReceivedFunctionWithSender:	user-defined function location in memory.
 *										specified when there are > 1 SerialPort instances
 *
 */
void SerialPort::setPacketHandler(PacketHandlerFunction PacketReceivedFunction)
{
	_PacketReceivedFunction = PacketReceivedFunction;
	_PacketReceivedFunctionWithSender = 0;
}
void SerialPort::setPacketHandler(PacketHandlerFunctionWithSender PacketReceivedFunctionWithSender)
{
	_PacketReceivedFunction = 0;
	_PacketReceivedFunctionWithSender = PacketReceivedFunctionWithSender;
}

/* Function flow:
 * --Reads in one byte at a time
 * --If the packetMarker is received, the function decodes the COBS encoded
 *   packet and executes the PacketReceivedFunction.
 * --Returns the number of bytes decoded.
 *
 * Function Params:
 * decodeBuffer:	The buffer into which the result will be written
 *
 * Function variables:
 * bytesRead:	total number of bytes read by ReadFile function in that read
 * data:		location where the read byte gets put by ReadFile
 * data_ptr:	pointer to data's location. Used to put byte into receiving buffer
 *
 */
int SerialPort::update(uint8_t *decodeBuffer)
{
    int		bytesAvailable;
    uint8_t 	data;
    uint8_t* 	data_ptr;
    data_ptr = 	& data;

	bytesAvailable = read(this->handler, &data, 1);
    
    if (bytesAvailable > 0)
	{		

       	if (data == PACKETMARKER)
    	{
    		size_t numDecoded = COBS::decode(_receiveBuffer,
									         _receiveBufferIndex,
									    	 decodeBuffer);									    	 					    	 													 	    	 								    	 
			// Execute whichever function was defined (with or w/o sender)
			if (_PacketReceivedFunction)
			{
				_PacketReceivedFunction(decodeBuffer, numDecoded);
			}
				
			else if (_PacketReceivedFunctionWithSender)
			{
				_PacketReceivedFunctionWithSender(this, _receiveBuffer, numDecoded);
			}

			// Clear the buffer						    	 
			memset(_receiveBuffer, 0, _receiveBufferIndex);
			_receiveBufferIndex = 0;
			return(numDecoded);
		}
		else
		{
			if ((_receiveBufferIndex + 1) < MAX_PACKET_LENGTH)
			{
				_receiveBuffer[_receiveBufferIndex++] = *data_ptr;
			}
		}
	}

    return 0;
}

/* Function flow:
 * --Send function takes a non-COBS encoded input, encodes it, and writes it
 *   to the serial line.
 * --Returns TRUE if the write was successful, FALSE if not.
 *
 * Function Params:
 * buffer:		The message that you want to encode and send
 * buf_size:	The size of the message in bytes
 *
 * Function variables:
 * bytesSend:	total number of bytes sent by WriteFile function in that write
 * encodedBuffer:	buffer to put the encoded message into
 * numEncoded:	number of bytes that were encoded. Does not include packet delimiter
 *
 */
bool SerialPort::send(uint8_t *buffer, size_t buf_size)
{
	uint8_t bytesSend;
    /* if the message is not empty & the size of the message wasn't 0 by accident */
    if (buffer != 0 && buf_size != 0)
	{
        uint8_t encodedBuffer[COBS::getEncodedBufferSize(buf_size)+1];

        size_t numEncoded = COBS::encode(buffer, buf_size, encodedBuffer);

        bytesSend = write(this->handler, (void*) encodedBuffer, numEncoded + 1);
        
		/* clear buffer */
		memset(encodedBuffer, 0, numEncoded + 1);
		
        return true;
    }
    else return false;
}

/* Function flow:
 * --Simply checks if the serial port is connected
 * --Returns the connection status of this instance of SerialPort
 *
 */
bool SerialPort::isConnected()
{
    return this->connected;
}

