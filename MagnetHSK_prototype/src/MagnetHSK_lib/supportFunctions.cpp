#include "supportFunctions.h"
// *****************
// Choose your SPI port
// *****************
SPIClass TM4CSPI(0);

// *****************
// Initializing the TM4C123 SPI connection
// *****************
void Initialize_TM4C123() {
  pinMode(TM4CSPI_clock, OUTPUT);
  digitalWrite(TM4CSPI_clock, 1);
  TM4CSPI.begin();
  pinMode(CHIP_SELECT, OUTPUT);
}

// *****************
// Measure channel
// *****************

// *********************************
// Extra in case power shuts off
// *********************************
int convert_channel(uint8_t chip_select, uint8_t channel_number) {
  // Start conversion
  transfer_byte(chip_select, WRITE_TO_RAM, COMMAND_STATUS_REGISTER,
                CONVERSION_CONTROL_BYTE | channel_number);
  int goAhead;
  goAhead = wait_for_process_to_finish(chip_select);
  return goAhead;
}

int wait_for_process_to_finish(uint8_t chip_select) {
  uint8_t process_finished = 0;
  uint8_t data;
  int goAhead = 1;

  // unsigned long CurrentTime;
  // unsigned long ElapsedTime;

  unsigned long StartTime = millis();

  while (process_finished == 0) {
    data =
        transfer_byte(chip_select, READ_FROM_RAM, COMMAND_STATUS_REGISTER, 0);
    process_finished = data & 0x40;
    if ((millis() - StartTime) > MaxWaitSpi) {
      goAhead = 0;
      return goAhead;
    }
  }
  // Serial.print(" t=");
  // Serial.print(millis() - StartTime);
  // Serial.print(" ");
  return goAhead;
}

float returnResistance(uint8_t chip_select, uint8_t channel_number) {

  int goAhead;
  goAhead = convert_channel(chip_select, channel_number);
  if (goAhead == 1) {
    int32_t raw_data;
    float voltage_or_resistance_result;
    uint16_t start_address = get_start_address(VOUT_CH_BASE, channel_number);

    raw_data =
        transfer_four_bytes(chip_select, READ_FROM_RAM, start_address, 0);
    voltage_or_resistance_result = (float)raw_data / 1024;
    return voltage_or_resistance_result;
  } else {
    return Nonsense;
  }
}

float returnTemperature(uint8_t chip_select, uint8_t channel_number) {
  int goAhead;
  goAhead = convert_channel(chip_select, channel_number);
  if (goAhead == 1) {
    uint32_t raw_data;
    uint8_t fault_data;
    uint16_t start_address =
        get_start_address(CONVERSION_RESULT_MEMORY_BASE, channel_number);
    uint32_t raw_conversion_result;
    int32_t signed_data;
    float scaled_result;

    raw_data =
        transfer_four_bytes(chip_select, READ_FROM_RAM, start_address, 0);

    // 24 LSB's are conversion result
    raw_conversion_result = raw_data & 0xFFFFFF;

    signed_data = raw_conversion_result;

    // Convert the 24 LSB's into a signed 32-bit integer
    if (signed_data & 0x800000)
      signed_data = signed_data | 0xFF000000;

    scaled_result = float(signed_data) / 1024;

    return scaled_result;
  } else {
    return -9999;
  }
}

// *********************************
// Get results
// *********************************
void read_voltage_or_resistance_results(uint8_t chip_select,
                                        uint8_t channel_number) {
  int32_t raw_data;
  float voltage_or_resistance_result;
  uint16_t start_address = get_start_address(VOUT_CH_BASE, channel_number);

  raw_data = transfer_four_bytes(chip_select, READ_FROM_RAM, start_address, 0);
  voltage_or_resistance_result = (float)raw_data / 1024;
//  Serial.print(F(" Resistance = "));
//  Serial.println(voltage_or_resistance_result);
}
//////////////////////////

// *********************
// SPI RAM data transfer
// *********************
// To write to the RAM, set ram_read_or_write = WRITE_TO_RAM.
// To read from the RAM, set ram_read_or_write = READ_FROM_RAM.
// input_data is the data to send into the RAM. If you are reading from the
// part, set input_data = 0.

uint32_t transfer_four_bytes(uint8_t chip_select, uint8_t ram_read_or_write,
                             uint16_t start_address, uint32_t input_data) {
  uint32_t output_data;
  uint8_t tx[7], rx[7];

  tx[6] = ram_read_or_write;
  tx[5] = highByte(start_address);
  tx[4] = lowByte(start_address);
  tx[3] = (uint8_t)(input_data >> 24);
  tx[2] = (uint8_t)(input_data >> 16);
  tx[1] = (uint8_t)(input_data >> 8);
  tx[0] = (uint8_t)input_data;

  spi_transfer_block(chip_select, tx, rx, 7);

  output_data = (uint32_t)rx[3] << 24 | (uint32_t)rx[2] << 16 |
                (uint32_t)rx[1] << 8 | (uint32_t)rx[0];

  return output_data;
}

uint8_t transfer_byte(uint8_t chip_select, uint8_t ram_read_or_write,
                      uint16_t start_address, uint8_t input_data) {
  uint8_t tx[4], rx[4];

  tx[3] = ram_read_or_write;
  tx[2] = (uint8_t)(start_address >> 8);
  tx[1] = (uint8_t)start_address;
  tx[0] = input_data;
  spi_transfer_block(chip_select, tx, rx, 4);
  return rx[0];
}

// Reads and sends a byte array
void spi_transfer_block(uint8_t cs_pin, uint8_t *tx, uint8_t *rx,
                        uint8_t length) {
  int8_t i;

  output_low(cs_pin); //! 1) Pull CS low

  for (i = (length - 1); i >= 0; i--)
    rx[i] = TM4CSPI.transfer(tx[i]); //! 2) Read and send byte array

  output_high(cs_pin); //! 3) Pull CS high
}

// ***********************
// Program the part
// ***********************
void assign_channel(uint8_t chip_select, uint8_t channel_number,
                    uint32_t channel_assignment_data) {
  uint16_t start_address = get_start_address(CH_ADDRESS_BASE, channel_number);
  transfer_four_bytes(chip_select, WRITE_TO_RAM, start_address,
                      channel_assignment_data);
}

void write_custom_table(uint8_t chip_select,
                        struct table_coeffs coefficients[64],
                        uint16_t start_address, uint8_t table_length) {
  int8_t i;
  uint32_t coeff;

  output_low(chip_select);

  TM4CSPI.transfer(WRITE_TO_RAM);
  TM4CSPI.transfer(highByte(start_address));
  TM4CSPI.transfer(lowByte(start_address));

  for (i = 0; i < table_length; i++) {
    coeff = coefficients[i].measurement;
    TM4CSPI.transfer((uint8_t)(coeff >> 16));
    TM4CSPI.transfer((uint8_t)(coeff >> 8));
    TM4CSPI.transfer((uint8_t)coeff);

    coeff = coefficients[i].temperature;
    TM4CSPI.transfer((uint8_t)(coeff >> 16));
    TM4CSPI.transfer((uint8_t)(coeff >> 8));
    TM4CSPI.transfer((uint8_t)coeff);
  }
  output_high(chip_select);
}

// ******************************
// Misc support functions
// ******************************
uint16_t get_start_address(uint16_t base_address, uint8_t channel_number) {
  return base_address + 4 * (channel_number - 1);
}
