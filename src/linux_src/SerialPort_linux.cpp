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

int SerialPort::readSerialPort(char *buffer)
{
	int bytesAvailable = read(this->handler, buffer, 255);

    return bytesAvailable;
}

bool SerialPort::writeSerialPort(char *buffer, unsigned int buf_size)
{
    int bytesSend = write(this->handler, buffer, buf_size);
	
	return bytesSend;
}

bool SerialPort::isConnected()
{
    return this->connected;
}

