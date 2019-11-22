// I2C functions for DCT HSK go here.
// 
// The following are I2C ports used on the TM4C123GXL launchpad for the DCT HSK:
// Pins    |    (5,6)      |      (9,10)     |    (23,24)
// I2C     | (SCL2,SDA2)   |    (SCL1,SDA1)  |  (SCL3,SDA3)
// Hardware| ADC_pressure  |    Cathode HV   |  Potential HV
#define PART_TM4C123GH6PM // This picks out the register values we need from the header files in the .cpp file.
// Slave address for I2C writes go here
#define DAC_slave_CATHODE 32
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <inc/hw_i2c.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>
#include <driverlib/i2c.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>


// LSB Weights
const float LTC2992_GPIO_8bit_lsb = 8.000E-3;                //!< Typical GPIO lsb weight for 8-bit mode in volts
const float LTC2992_GPIO_12bit_lsb = 0.500E-3;                //!< Typical GPIO lsb weight for 12-bit mode in volts
const float LTC2992_DELTA_SENSE_8bit_lsb = 200.00E-6;        //!< Typical Delta lsb weight for 8-bit mode in volts
const float LTC2992_DELTA_SENSE_12bit_lsb = 12.50E-6;        //!< Typical Delta lsb weight for 12-bit mode in volts
const float LTC2992_SENSE_8bit_lsb = 400.00E-3;                //!< Typical SENSE lsb weight for 8-bit mode in volts
const float LTC2992_SENSE_12bit_lsb = 25.00E-3;                //!< Typical SENSE lsb weight for 12-bit mode in volts
const float LTC2992_Power_8bit_lsb = LTC2992_DELTA_SENSE_8bit_lsb*LTC2992_SENSE_8bit_lsb;     //!< Typical POWER lsb weight for 8-bit mode in V^2
const float LTC2992_Power_12bit_lsb = LTC2992_DELTA_SENSE_12bit_lsb*LTC2992_SENSE_12bit_lsb;     //!< Typical POWER lsb weight for 12-bit mode in V^2
const float resistor = .01;         //!< resistor value on demo board

// ******************************
// I2C specific functions
// ******************************
// Opens I2C port 0 which are pins PB2 and PB3
void InitI2C0(void);
// Opens port of choice (0,3)
void InitI2C(const int port);
// Sends I2C N bytes to slave address on port of choice
uint32_t I2CSendNbytes(const int base, uint8_t slave_addr, uint8_t num_of_args, ...);
// Receives I2C N bytes to slave address on port of choice
uint32_t I2CReadNbytes(const int base, uint8_t slave_addr, uint8_t num_of_args, uint8_t reg, uint8_t * readStore);


//! This union splits one int32_t (32-bit signed integer) or uint32_t (32-bit unsigned integer)
//! four uint8_t's (8-bit unsigned integers) and vice versa.
union LT_union_int32_4bytes
{
  int32_t LT_int32;       //!< 32-bit signed integer to be converted to four bytes
  uint32_t LT_uint32;     //!< 32-bit unsigned integer to be converted to four bytes
  uint8_t LT_byte[4];     //!< 4 bytes (unsigned 8-bit integers) to be converted to a 32-bit signed or unsigned integer
  uint16_t LT_2byte[2];
};