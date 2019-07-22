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

    /* Set I/O options location & baud rate speed */
    cfsetospeed(&options, (speed_t)speed);
    cfsetispeed(&options, (speed_t)speed);
    
    /* 8 bits, no parity, no stop bits */
 	options.c_cflag &= ~PARENB;
 	options.c_cflag &= ~CSTOPB;
 	options.c_cflag &= ~CSIZE;
 	options.c_cflag |= CS8;
 	
 	cfmakeraw(&options);
//  	/* no hardware flow control */
 	options.c_cflag &= ~CRTSCTS;
//  	/* enable receiver, ignore status lines */
 	options.c_cflag |= CREAD | CLOCAL;
 	
 	/* disable input/output flow control, disable restart chars */
 	options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);
 	
//  	/* disable canonical input, disable echo,
//  	disable visually erase chars,
//  	disable terminal-generated signals */
 	options.c_lflag &= ~(ICANON | ECHO | ECHONL | ECHOE | ISIG | IEXTEN);
 	
//  	/* disable output processing */
 	options.c_oflag &= ~OPOST;

    /* fetch bytes as they become available w/.1 second timeout */
    options.c_cc[VMIN] = 0;		// VMIN = min # bytes to be available before read
    options.c_cc[VTIME] = 1;	// VTIME = timeout in tenths of seconds
    
    /* flush the input buffer, prep for new communication. I don't think this works */
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
    options.c_cc[VTIME] = 1;

    if (tcsetattr(portName, TCSANOW, &options) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}
