#include "LinuxLib.h"

using std::cout;
using std::endl;
using std::cin;

int set_interface_attribs (int portName, int speed, int parity)
{
	struct termios options;

    if (tcgetattr(portName, &options) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&options, (speed_t)speed);
    cfsetispeed(&options, (speed_t)speed);

    options.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;         /* 8-bit characters */
    options.c_cflag &= ~PARENB;     /* no parity bit */
    options.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    options.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    options.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;
    
    /* flush the input buffer */
    tcflush(portName, TCIFLUSH);


    if (tcsetattr(portName, TCSANOW, &options) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

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


//int main()
//{
	//int portName = open ("/dev/optionsS0", O_RDWR | O_NOCoptions | O_SYNC);
	
	//set_interface_attribs (portName, B9600, 0);  // set speed to 9,600 bps, 8n1 (no parity)
	//set_blocking (portName, 0);
		
	//cout << "HEY" << endl; 
	//sleep(2);
	//char x;
	//cout << "Hello world" << endl;
	//sleep(2);
	//cin >> x;
	//cout << "Your input: " << x << endl;
	//sleep(2);
	
//}
