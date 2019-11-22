#pragma once

#include "Core_protocol.h"
#include <iostream>


#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>


/* These are device specific */
#include "LTC2992.h"

// LSB Weights
const float LTC2992_GPIO_8bit_lsb = 8.000E-3;                //!< Typical GPIO lsb weight for 8-bit mode in volts
const float LTC2992_GPIO_12bit_lsb = 0.500E-3;                //!< Typical GPIO lsb weight for 12-bit mode in volts
const float LTC2992_DELTA_SENSE_8bit_lsb = 200.00E-6;        //!< Typical Delta lsb weight for 8-bit mode in volts
const float LTC2992_DELTA_SENSE_12bit_lsb = 12.50E-6;        //!< Typical Delta lsb weight for 12-bit mode in volts
const float LTC2992_SENSE_8bit_lsb = 400.00E-3;                //!< Typical SENSE lsb weight for 8-bit mode in volts
const float LTC2992_SENSE_12bit_lsb = 25.00E-3;                //!< Typical SENSE lsb weight for 12-bit mode in volts
const float LTC2992_Power_8bit_lsb = LTC2992_DELTA_SENSE_8bit_lsb*LTC2992_SENSE_8bit_lsb;     //!< Typical POWER lsb weight for 8-bit mode in V^2
const float LTC2992_Power_12bit_lsb = LTC2992_DELTA_SENSE_12bit_lsb*LTC2992_SENSE_12bit_lsb;     //!< Typical POWER lsb weight for 12-bit mode in V^2

#define CONTINUOUS_MODE_DISPLAY_DELAY 3000                  //!< The delay between readings in continious mode


const float resistor = .01;         //!< resistor value on demo board

const int port_0 = 0;

//! This union splits one int32_t (32-bit signed integer) or uint32_t (32-bit unsigned integer)
//! four uint8_t's (8-bit unsigned integers) and vice versa.
union LT_union_int32_4bytes
{
  int32_t LT_int32;       //!< 32-bit signed integer to be converted to four bytes
  uint32_t LT_uint32;     //!< 32-bit unsigned integer to be converted to four bytes
  uint8_t LT_byte[4];     //!< 4 bytes (unsigned 8-bit integers) to be converted to a 32-bit signed or unsigned integer
  uint16_t LT_2byte[2];
};

// Calculate the LTC2992 SENSE voltage
float LTC2992_SENSE_code_to_voltage(uint16_t adc_code, float LTC2992_SENSE_lsb);

// Calculate the LTC2992 GPIO voltage
float LTC2992_GPIO_code_to_voltage(uint16_t adc_code, float LTC2992_GPIO_lsb);

// Calculate the LTC2992 current with a sense resistor
float LTC2992_code_to_current(uint16_t adc_code, float resistor, float LTC2992_DELTA_SENSE_lsb);

// Calculate the LTC2992 current with a sense resistor
float LTC2992_code_to_current_sum(uint16_t adc_code, float resistor, float LTC2992_DELTA_SENSE_lsb);

// Calculate the LTC2992 power
float LTC2992_code_to_power(int32_t adc_code, float resistor, float LTC2992_Power_lsb);

// Calculate the LTC2992 power
float LTC2992_code_to_power_sum(int32_t adc_code, float resistor, float LTC2992_Power_lsb);