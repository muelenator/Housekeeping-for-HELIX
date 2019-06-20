/*
 * LinuxLib.cpp
 *
 * Provides helper functions for SerialPort_linux. Sets up the serial 
 * connection and preps the serial bus.
 * 
 * Function definitions
 *
 */



#include "LinuxLib.h"

using std::cout;
using std::endl;
using std::cin;

/*****************************************************************************
 * Functions
 ****************************************************************************/
 
/* Function flow
 * --Creates a linux terminal struct (termios options) to put serial bus 
	 information into
 * --Sets specific serial line flags to enable raw input + output
 * 
 * Function params:
 * portName			Name of our opened serial port in SerialPort_linux
 * speed			Baudrate of the proposed serial communication
 * parity			Specifies the parity (here is no parity)
 *
 * Function variables:
 * options:			A POSIX standard struct which stores our serial options 
 *
 */
int set_interface_attribs (int portName, int speed, int parity)
{
	struct termios options;

    if (tcgetattr(portName, &options) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&options, (speed_t)speed);
    cfsetispeed(&options, (speed_t)speed);

    options.c_cflag |= (CLOCAL | CREAD);    // ignore modem controls
    options.c_cflag &= ~CSIZE;				// set the bit-mask
    options.c_cflag |= CS8;         		// 8-bit characters
    options.c_cflag &= ~PARENB;    			// no parity bit
    options.c_cflag &= ~CSTOPB;     		// only need 1 stop bit
    options.c_cflag &= ~CRTSCTS;    		// no hardware flowcontrol

    /* setup for non-canonical mode--turn off most flags */
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    options.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    options.c_cc[VMIN] = 0;		// VMIN = min # bytes to be available before read
    options.c_cc[VTIME] = 0;	// VTIME = timeout in tenths of seconds
    
    /* flush the input buffer, prep for new communication */
    tcflush(portName, TCIFLUSH);

	/* set all these new options. If they don't set, spit out an error */
    if (tcsetattr(portName, TCSANOW, &options) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

/* Function flow
 * --Set blocking variables VMIN & VTIME on the fly
 * 
 * Function params:
 * portName			Name of our opened serial port in SerialPort_linux 
 * mcount 			The proposed minimum # bytes that can be used by our program
 *
 * Function variables:
 * options:			A POSIX standard struct which stores our serial options 
 *
 */
/* Set blocking variables VMIN & VTIME on the fly */
void set_mincount(int portName, int mcount)
{
    struct termios options;

    if (tcgetattr(portName, &options) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    options.c_cc[VMIN] = mcount ? 1 : 0;
    options.c_cc[VTIME] = 5;        /* half second timer */

    if (tcsetattr(portName, TCSANOW, &options) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}
