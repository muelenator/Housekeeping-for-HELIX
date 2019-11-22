#include "SolarFunctions.h"

// Calculate the LTC2992 SENSE voltage
float LTC2992_SENSE_code_to_voltage(uint16_t adc_code, float LTC2992_SENSE_lsb)
// Returns the SENSE Voltage in Volts
{
  float voltage;
  voltage = (float)adc_code*LTC2992_SENSE_lsb;    //! 1) Calculate voltage from code and lsb
  return(voltage);
}

// Calculate the LTC2992 GPIO voltage
float LTC2992_GPIO_code_to_voltage(uint16_t adc_code, float LTC2992_GPIO_lsb)
// Returns the GPIO Voltage in Volts
{
  float adc_voltage;
  adc_voltage = (float)adc_code*LTC2992_GPIO_lsb;   //! 1) Calculate voltage from code and ADIN lsb
  return(adc_voltage);
}

// Calculate the LTC2992 current with a sense resistor
float LTC2992_code_to_current(uint16_t adc_code, float resistor, float LTC2992_DELTA_SENSE_lsb)
// Returns the LTC2992 current in Amps
{
  float voltage, current;
  voltage = (float)adc_code*LTC2992_DELTA_SENSE_lsb;    //! 1) Calculate voltage from ADC code and delta sense lsb
  current = voltage/resistor;                           //! 2) Calculate current, I = V/R
  return(current);
}

// Calculate the LTC2992 current with a sense resistor
float LTC2992_code_to_current_sum(uint16_t adc_code, float resistor, float LTC2992_DELTA_SENSE_lsb)
// Returns the LTC2992 current in Amps
{
  float voltage, current;
  voltage = (float)(adc_code<<1)*LTC2992_DELTA_SENSE_lsb;    //! 1) Calculate voltage from ADC code and delta sense lsb
  current = voltage/resistor;                           //! 2) Calculate current, I = V/R
  return(current);
}

// Calculate the LTC2992 power
float LTC2992_code_to_power(int32_t adc_code, float resistor, float LTC2992_Power_lsb)
// Returns The LTC2992 power in Watts
{
  float power;
  power = (float)adc_code*LTC2992_Power_lsb/resistor;  //! 1) Calculate Power using Power lsb and resistor

  return(power);
}

// Calculate the LTC2992 power
float LTC2992_code_to_power_sum(int32_t adc_code, float resistor, float LTC2992_Power_lsb)
// Returns The LTC2992 power in Watts
{
  float power;
  power = (float)(adc_code<<1)*LTC2992_Power_lsb/resistor;  //! 1) Calculate Power using Power lsb and resistor

  return(power);
}