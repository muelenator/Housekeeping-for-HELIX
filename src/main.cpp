/*
 * main.cpp
 *
 * Basic run of input/output for LaunchPad communication. All applicability is
 * defined in userTest.cpp. Only the USB port address is intended to be editted
 * in this file.
 *
 * --Serial port name on Windows. If, in 'COM#', # is > 9, portName must include
 *   these backslashes: '\\\\.\\COM#'
 */

/*******************************************************************************
 * Defines
 *******************************************************************************/
#ifdef _WIN32
#include "win_src/SerialPort.cpp"
#include "win_src/SerialPort.h"
#endif

#ifdef __linux__
#include "linux_src/LinuxLib.cpp"
#include "linux_src/LinuxLib.h"
#include "linux_src/SerialPort_linux.cpp"
#include "linux_src/SerialPort_linux.h"
#endif

#include <cerrno>
#include <chrono>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "board_functions/Core_protocol.h"
#include "board_functions/Core_protocol.cpp"
#include "userTest.h"

#include "board_functions/SolarFunctions.h"
#include "board_functions/SolarHSK_protocol.h"
#include "board_functions/SolarFunctions.cpp"
#include "board_functions/MagnetFunctions.h"
#include "board_functions/MagnetFunctions.cpp"

#include <fstream>


using std::cin;
using std::cout;
using std::endl;

extern uint8_t cinNumber();

/******************************************************************************/
/* Serial port parameters */
const char *port_name = "\\\\.\\COM10";
int SerialBaud = 115200;
/******************************************************************************/

/* Name this device */
housekeeping_id myComputer = eSFC;

/* Declarations to keep track of device list */
uint8_t downStreamDevices[254] = {0}; // Keep a list of downstream devices
uint8_t numDevices = 0; // Keep track of how many devices are downstream

/* Keep a log of errors */
uint8_t errorsReceived[254] = {0};
uint8_t numErrors = 0;

/* Create buffers for data */
uint8_t outgoingPacket[MAX_PACKET_LENGTH] = {0}; // Buffer for outgoing packet
uint8_t incomingPacket[MAX_PACKET_LENGTH] = {0}; // Buffer for incoming packet

/* Defining the variable here makes the linker connect where these variables
 * are being used (see iProtocol.h for declaration)	*/
housekeeping_hdr_t *hdr_in;
housekeeping_hdr_t *hdr_out;
housekeeping_err_t *hdr_err;
housekeeping_prio_t *hdr_prio;

/* Utility variableas: */
uint8_t checkin;         // Checksum value for read-in
uint8_t lengthBeingSent; // Actual

uint16_t numberIN;
std::string numbufIN;

/* While connected, variables to check the how long its been since read a byte
 */
int read_result = 0;
std::chrono::time_point<std::chrono::system_clock> newest_zero, newest_result;
std::chrono::duration<double> elapsed_time;

/* Set up a delay */
std::chrono::time_point<std::chrono::system_clock> timeSinceDelay;
std::chrono::duration<double> delayed_time;
bool delayOver = true;
double userTIME;

/* Bool if a reset needs to happen */
bool needs_reset = false;

/*******************************************************************************
 * Functions
 *******************************************************************************/
/* Function flow:
 * --Gets outgoing header from the userTest function 'setupMyPacket'
 * --Computes the checksum of the outgoing data
 * --Asks user what checksum to use
 *
 * Function variables:
 * numbufIN:		Character buffer for user input
 * numberIN:		Unsigned integer that resulted from the conversion
 *
 */
bool setup() {
  cout << "Standby mode? (Type 0 for no delay, or enter an integer # of "
          "seconds)";
  cout << endl;
  if (userTIME = (double)cinNumber()) {
    delayOver = false;
    timeSinceDelay = std::chrono::system_clock::now();
    return false;
  }
  delayOver = true;

  lengthBeingSent = setupMyPacket(hdr_out, hdr_prio); // Fills in rest of header

  /* Compute checksum for outgoing packet + add it to the end of packet */
  fillChecksum((uint8_t *)outgoingPacket);

  cout << "The computed checksum is "
       << (int)outgoingPacket[4 + lengthBeingSent];
  cout << ". Enter this value, or input a different checksum to check protocol "
          "reliability.";
  cout << endl;

  outgoingPacket[4 + lengthBeingSent] = cinNumber();
  cout << endl;
  
  return true;
}

/* Function flow:
 * --Checks to see if the packet was received from an unknown device
 *		--If so, add that device address to the list of known devices
 * --Sorts through commands & executes defined functions in myprogram.h
 * --If there is no protocol for a command, just reads header + displays data
 *
 * Function params:
 * buffer:		Pointer to the location of the incoming packet
 *
 * Function variables:
 * downStreamDevices:		The array of addresses of known devices
 * numDevices:				Running total of downstream devices
 *
 */
void commandCenter(const uint8_t *buffer) {
  /* Check if the device is already known */
  if (findMe(downStreamDevices, downStreamDevices + numDevices, hdr_in->src) ==
      downStreamDevices + numDevices) {
    /* If not, add it to the list of known devices */
    downStreamDevices[numDevices] = hdr_in->src;
    numDevices += 1;
  }
  
  justReadHeader(hdr_in);
  
  if (hdr_in->cmd == ePingPong) {
		cout << endl;
		return;
  } else if ((int)hdr_in->cmd < eSendAll &&
             (int)hdr_in->cmd >= eSendLowPriority && hdr_in->len == 0) {
		cout << "Device #" << (int)hdr_in->src;
		cout << " did not have any data of this priority." << endl << endl;
		return;
  } else if (hdr_in->cmd == eError) {
		whatToDoIfError(hdr_err, errorsReceived, numErrors);	
		return;
  } else if (hdr_in->cmd == eSetPriority) {
		whatToDoIfSetPriority(hdr_in, hdr_prio);
		return;
  }
  
  uint8_t* datapac = (uint8_t*) (buffer+4);
  
  switch (hdr_in->src) {
	  case eMainHsk:
		if (hdr_in->cmd==eMapDevices) whatToDoIfMap(hdr_in);
		else if (hdr_in->cmd==eIntSensorRead) whatToDoIfISR(hdr_in);
		break;
	  case eMagnetHsk:
		if (hdr_in->cmd >=2 && hdr_in->cmd<=13) whatToDoIfFloat(hdr_in);
		else if (hdr_in->cmd ==14 || hdr_in->cmd==15) whatToDoIfFlow(hdr_in);
		else if (hdr_in->cmd >=16 && hdr_in->cmd <=25) whatToDoIfTempProbes(hdr_in);
		else if (hdr_in->cmd ==26) whatToDoIfPressure(hdr_in);
		break;
	  case eDCTHsk:
		if (hdr_in->cmd==hdr_in->cmd) whatToDoIfThermistorsTest(hdr_in);
		break;
	  case eSolarHsk:
		switch (hdr_in->cmd) {
		  case ePower1:
			float power1, max_power1, min_power1;
			power1 = LTC2992_code_to_power(*(uint32_t*)datapac, resistor, LTC2992_Power_12bit_lsb);
			max_power1 = LTC2992_code_to_power(*(uint32_t*)(datapac+4), resistor, LTC2992_Power_12bit_lsb);
			min_power1 = LTC2992_code_to_power(*(uint32_t*)(datapac+8), resistor, LTC2992_Power_12bit_lsb);
			cout << "Power1:      " << power1 << endl;
			cout << "Max Power1:  " << max_power1 << endl;
			cout << "Min Power1:  " << min_power1 << endl;
			break;
		  case ePower2:
			float power2, max_power2, min_power2;
			power2 = LTC2992_code_to_power(*(uint32_t*)datapac, resistor, LTC2992_Power_12bit_lsb);
			max_power2 = LTC2992_code_to_power(*(uint32_t*)(datapac+4), resistor, LTC2992_Power_12bit_lsb);
			min_power2 = LTC2992_code_to_power(*(uint32_t*)(datapac+8), resistor, LTC2992_Power_12bit_lsb);
			cout << "Power2:      " << power2 << endl;
			cout << "Max Power2:  " << max_power2 << endl;
			cout << "Min Power2:  " << min_power2 << endl;
			break;
			
		  case eSense1:
		    float SENSE1, max_SENSE1, min_SENSE1;
			SENSE1 = LTC2992_SENSE_code_to_voltage(*(uint16_t*)datapac, LTC2992_SENSE_12bit_lsb);
			max_SENSE1 = LTC2992_SENSE_code_to_voltage(*(uint16_t*)(datapac+2), LTC2992_SENSE_12bit_lsb);
			min_SENSE1 = LTC2992_SENSE_code_to_voltage(*(uint16_t*)(datapac+4), LTC2992_SENSE_12bit_lsb);
			cout << "Voltage1:      " << SENSE1 << endl;
			cout << "Max Voltage1:  " << max_SENSE1 << endl;
			cout << "Min Volatge1:  " << min_SENSE1 << endl;
			break;
		  case eSense2:
			float SENSE2, max_SENSE2, min_SENSE2;
			SENSE2 = LTC2992_SENSE_code_to_voltage(*(uint16_t*)datapac, LTC2992_SENSE_12bit_lsb);
			max_SENSE2 = LTC2992_SENSE_code_to_voltage(*(uint16_t*)(datapac+2), LTC2992_SENSE_12bit_lsb);
			min_SENSE2 = LTC2992_SENSE_code_to_voltage(*(uint16_t*)(datapac+4), LTC2992_SENSE_12bit_lsb);
			cout << "Voltage2:      " << SENSE2 << endl;
			cout << "Max Voltage2:  " << max_SENSE2 << endl;
			cout << "Min Volatge2:  " << min_SENSE2 << endl;
			break;
			
		  case eCurrent1:
			float current1, max_current1, min_current1;
			current1 = LTC2992_code_to_current(*(uint16_t*)datapac, resistor, LTC2992_DELTA_SENSE_12bit_lsb);
			max_current1 = LTC2992_code_to_current(*(uint16_t*)(datapac+2), resistor, LTC2992_DELTA_SENSE_12bit_lsb);
			min_current1 = LTC2992_code_to_current(*(uint16_t*)(datapac+4), resistor, LTC2992_DELTA_SENSE_12bit_lsb);
			cout << "Current1:      " << current1 << endl;
			cout << "Max Current1:  " << max_current1 << endl;
			cout << "Min Current1:  " << min_current1 << endl;
			break;
		 case eCurrent2:
			float current2, max_current2, min_current2;
			current2 = LTC2992_code_to_current(*(uint16_t*)datapac, resistor, LTC2992_DELTA_SENSE_12bit_lsb);
			max_current2 = LTC2992_code_to_current(*(uint16_t*)(datapac+2), resistor, LTC2992_DELTA_SENSE_12bit_lsb);
			min_current2 = LTC2992_code_to_current(*(uint16_t*)(datapac+4), resistor, LTC2992_DELTA_SENSE_12bit_lsb);
			cout << "Current2:      " << current2 << endl;
			cout << "Max Current2:  " << max_current2 << endl;
			cout << "Min Current2:  " << min_current2 << endl;
			break;
			
		  case eGPIO1:
			float GPIO1, max_GPIO1, min_GPIO1;
			GPIO1 = LTC2992_GPIO_code_to_voltage(*(uint16_t*)datapac, LTC2992_GPIO_12bit_lsb);
			max_GPIO1 = LTC2992_GPIO_code_to_voltage(*(uint16_t*)(datapac+2), LTC2992_GPIO_12bit_lsb);
			min_GPIO1 = LTC2992_GPIO_code_to_voltage(*(uint16_t*)(datapac+4), LTC2992_GPIO_12bit_lsb);
			cout << "GPIO1 Voltage:      " << GPIO1 << endl;
			cout << "Max GPIO1 Voltage:  " << max_GPIO1 << endl;
			cout << "Min GPIO1 Volatge:  " << min_GPIO1 << endl;
			break;
		  case eGPIO2:
		    float GPIO2, max_GPIO2, min_GPIO2;
			GPIO2 = LTC2992_GPIO_code_to_voltage(*(uint16_t*)datapac, LTC2992_GPIO_12bit_lsb);
			max_GPIO2 = LTC2992_GPIO_code_to_voltage(*(uint16_t*)(datapac+2), LTC2992_GPIO_12bit_lsb);
			min_GPIO2 = LTC2992_GPIO_code_to_voltage(*(uint16_t*)(datapac+4), LTC2992_GPIO_12bit_lsb);
			cout << "GPIO2 Voltage:      " << GPIO2 << endl;
			cout << "Max GPIO2 Voltage:  " << max_GPIO2 << endl;
			cout << "Min GPIO2 Volatge:  " << min_GPIO2 << endl;
			break;
		  case eGPIO3:
		    float GPIO3, max_GPIO3, min_GPIO3;
			GPIO3 = LTC2992_GPIO_code_to_voltage(*(uint16_t*)datapac, LTC2992_GPIO_12bit_lsb);
			max_GPIO3 = LTC2992_GPIO_code_to_voltage(*(uint16_t*)(datapac+2), LTC2992_GPIO_12bit_lsb);
			min_GPIO3 = LTC2992_GPIO_code_to_voltage(*(uint16_t*)(datapac+4), LTC2992_GPIO_12bit_lsb);
			cout << "GPIO3 Voltage:      " << GPIO3 << endl;
			cout << "Max GPIO3 Voltage:  " << max_GPIO3 << endl;
			cout << "Min GPIO3 Volatge:  " << min_GPIO3 << endl;
			break;
		  case eGPIO4:
			float GPIO4, max_GPIO4, min_GPIO4;
			GPIO4 = LTC2992_GPIO_code_to_voltage(*(uint16_t*)datapac, LTC2992_GPIO_12bit_lsb);
			max_GPIO4 = LTC2992_GPIO_code_to_voltage(*(uint16_t*)(datapac+2), LTC2992_GPIO_12bit_lsb);
			min_GPIO4 = LTC2992_GPIO_code_to_voltage(*(uint16_t*)(datapac+4), LTC2992_GPIO_12bit_lsb);
			cout << "GPIO4 Voltage:      " << GPIO4 << endl;
			cout << "Max GPIO4 Voltage:  " << max_GPIO4 << endl;
			cout << "Min GPIO4 Volatge:  " << min_GPIO4 << endl;
			break;
		}
		cout << endl;
		break;	  
	  
	  default:
		justReadHeader(hdr_in);
		cout << "DATA: ";

		for (int i = 0; i < hdr_in->len; i++) {
			cout << (int)*((uint8_t *)hdr_in + 4 + i) << " ";
		}
		cout << endl << endl;
  }
}

/* Function flow:
 * --Called when a packet is received by the serial port instance
 * --Checks to see if the packet is meant for this device
 *		--If so, check the checksum for data corruption +
 *		  execute the command in the header
 *
 * Function params:
 * buffer:		Pointer to the location of the incoming packet
 * len:			Size of the decoded incoming packet
 *
 * Function variables:
 * checkin:		Utility variable to store a checksum in
 *
 */
void checkHdr(const uint8_t *buffer, size_t len) {
  /* Check if the message was intended for this device
   * If it was, check and execute the command  */
  if (hdr_in->dst == myComputer) {
    if (verifyChecksum((uint8_t *)buffer)) {
      commandCenter(buffer);
    } else
      cout << "Bummer, checksum did not match." << endl;
  }

  /* If it wasn't, cast it as an error & restart */
  else {
    cout << "Bad destination received... Restarting downstream devices."
         << endl;

    resetAll(hdr_out);

    /* Compute checksum for outgoing packet + add it to the end of packet */
    fillChecksum((uint8_t *)outgoingPacket);
    needs_reset = true;
  }
}

/*******************************************************************************
 * Main program
 *******************************************************************************/
int main() {

//  ofstream myfile;
//  myfile.open("bugs_test.txt");
  /* Point to data in a way that it can be read as known data structures */
  hdr_in = (housekeeping_hdr_t *)incomingPacket;
  hdr_out = (housekeeping_hdr_t *)outgoingPacket;
  hdr_err = (housekeeping_err_t *)(incomingPacket + 4);
  hdr_prio = (housekeeping_prio_t *)(incomingPacket + 4);

  /* Create the header for the first message */
  hdr_out->src = myComputer; // Source of data packet

  /* Declare an instance of the serial port connection */
  SerialPort TM4C(port_name, SerialBaud);

  /* Check if connection is established */
  if (TM4C.isConnected())
    cout << "Connection Established" << endl;
  else {
    cout << "ERROR, check port name";
    return 0;
  }

  /* Set the function that will act when a packet is received */
  TM4C.setPacketHandler(&checkHdr);

  /* Start up your program & set the outgoing packet data + send it out */
  startUp(hdr_out);
  fillChecksum((uint8_t *)outgoingPacket);
  TM4C.send(outgoingPacket, 4 + hdr_out->len + 1);

  /* On startup: Reset number of found devices & errors to 0 */
  memset(downStreamDevices, 0, numDevices);
  numDevices = 0;
  memset(errorsReceived, 0, numErrors);
  numErrors = 0;

  /* Initialize timing variables for when the last message was received */
  newest_zero = std::chrono::system_clock::now();
  newest_result = std::chrono::system_clock::now();

  /* While the serial port is open, */
  while (TM4C.isConnected()) {
    /* Reads in 1 byte at a time until the full packet arrives.
     * If a full packet is received, update will execute PacketReceivedFunction
     * If no full packet is received, bytes are discarded   */
    read_result = TM4C.update(incomingPacket);

    /* If a packet was decoded, mark down the time it happened */
    if (read_result > 0)
      newest_result = std::chrono::system_clock::now();

    /* If a packet wasn't decoded in this run, write down the current time */
    else
      newest_zero = std::chrono::system_clock::now();

    /* Using the above variable definitions, calculate how long its been since
     * something has been decoded */
    elapsed_time = newest_zero - newest_result;
    delayed_time = std::chrono::system_clock::now() - timeSinceDelay;
    if (delayed_time.count() > userTIME) {
      delayOver = true;
    }

    /* If that ^ time is greater than 1/2 a second, prompt the user again */
    if (elapsed_time.count() > .5 && delayOver) {
      /* Check if a reset needs to be sent */
      if (needs_reset) {
        TM4C.send(outgoingPacket, 4 + hdr_out->len + 1);
        needs_reset = false;
        return 0;
      }

      /* If it doesn't, prompt the user again for packet params  */
      if (setup()) {
        /* Send out the header and packet*/
        TM4C.send(outgoingPacket, 4 + lengthBeingSent + 1);

        /* Reset the timing system */
        newest_zero = std::chrono::system_clock::now();
        newest_result = std::chrono::system_clock::now();
      }
    }
  }
}
