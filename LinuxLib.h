#pragma once

#include <iostream>

#include <cstdint>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

int set_interface_attribs (int portName, int speed, int parity);

void set_mincount (int portName, int should_block);
