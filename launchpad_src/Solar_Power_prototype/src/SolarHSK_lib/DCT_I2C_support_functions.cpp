// Include I2C peripherals initialization and writing, receiving functions for pins on DCT HSK
////FOR I2C comms on TM4C///
#include "DCT_I2C_support_functions.h"
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

// *****************
// Initialize some I2C ports
// *****************
//initialize I2C module 0
//Slightly modified version of TI's example code
// Borrowed this from a digikey page and made my own changes.
void InitI2C0(void)
{
    //enable I2C module 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
 
    //reset module
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);
     
    //enable GPIO peripheral that contains I2C 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
 
    // Configure the pin muxing for I2C0 functions on port B2 and B3.
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);
     
    // Select the I2C function for these pins.
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);
 
    // Enable and initialize the I2C0 master module.  Use the system clock for
    // the I2C0 module.  The last parameter sets the I2C data transfer rate.
    // If false the data rate is set to 100kbps and if true the data rate will
    // be set to 400kbps.
    I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), false);
     
    //clear I2C FIFOs
    HWREG(I2C0_BASE + I2C_O_FIFOCTL) = 80008000;
}
void InitI2C(const int port)
{
    // For any I2C port specified that TM4C has. 
    if(port==0){
        SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
        SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
        GPIOPinConfigure(GPIO_PB2_I2C0SCL);
        GPIOPinConfigure(GPIO_PB3_I2C0SDA);
        GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
        GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);
        I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), false);
        HWREG(I2C0_BASE + I2C_O_FIFOCTL) = 80008000;
    }
    else if(port==1){
        SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
        SysCtlPeripheralReset(SYSCTL_PERIPH_I2C1);
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
        GPIOPinConfigure(GPIO_PA6_I2C1SCL);
        GPIOPinConfigure(GPIO_PA7_I2C1SDA);
        GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
        GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);
        I2CMasterInitExpClk(I2C1_BASE, SysCtlClockGet(), false);
        HWREG(I2C1_BASE + I2C_O_FIFOCTL) = 80008000;
    }
    else if(port==2){
        SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);
        SysCtlPeripheralReset(SYSCTL_PERIPH_I2C2);
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
        GPIOPinConfigure(GPIO_PE4_I2C2SCL);
        GPIOPinConfigure(GPIO_PE5_I2C2SDA);
        GPIOPinTypeI2CSCL(GPIO_PORTE_BASE, GPIO_PIN_4);
        GPIOPinTypeI2C(GPIO_PORTE_BASE, GPIO_PIN_5);
        I2CMasterInitExpClk(I2C2_BASE, SysCtlClockGet(), false);
        HWREG(I2C2_BASE + I2C_O_FIFOCTL) = 80008000;
    }
    else if(port==3){
        SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C3);
        SysCtlPeripheralReset(SYSCTL_PERIPH_I2C3);
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
        GPIOPinConfigure(GPIO_PD0_I2C3SCL);
        GPIOPinConfigure(GPIO_PD1_I2C3SDA);
        GPIOPinTypeI2CSCL(GPIO_PORTD_BASE, GPIO_PIN_0);
        GPIOPinTypeI2C(GPIO_PORTD_BASE, GPIO_PIN_1);
        I2CMasterInitExpClk(I2C3_BASE, SysCtlClockGet(), false);
        HWREG(I2C3_BASE + I2C_O_FIFOCTL) = 80008000;
    }

}
// *****************
// Send a variable number of bytes as a write to slave
// *****************
//sends an I2C command to the specified slave. See page 1001 of the microcontroller manual
uint32_t I2CSendNbytes(const int base,uint8_t slave_addr, uint8_t num_of_args, ...)
{
    uint32_t error_return=0; // error defs comments at end of function
    uint32_t base_to_use;
	switch (base) {
		case 0: base_to_use=I2C0_BASE;
			break;
		case 1: base_to_use=I2C1_BASE;
			break;
		case 2: base_to_use=I2C2_BASE;
			break;
		case 3: base_to_use=I2C3_BASE;
			break;
		
		default: return (uint8_t) 69;
	}
	
    I2CMasterSlaveAddrSet(base_to_use, slave_addr, false); // 7 bit slave address. 
     
    //stores list of variable number of arguments
    va_list vargs;
     
    // specifies the va_list to "open" and the last fixed argument
    // so vargs knows where to start looking
    va_start(vargs, num_of_args);
    // Step 2 of sec 16.3.1.5 
    // put data to be sent into FIFO
    // (I checked and it needs to be cast as uin32_t). 
    I2CMasterDataPut(base_to_use, va_arg(vargs, uint32_t));
     
    //if there is only one argument, we only need to use the
    //single send I2C function
    if(num_of_args == 1)
    {
        //Initiate send of data from the MCU
        // if only argument, single send, then not the section im currently commenting
        I2CMasterControl(base_to_use, I2C_MASTER_CMD_SINGLE_SEND);
         
        // Wait until MCU is done transferring.
        while(I2CMasterBusy(base_to_use));
        error_return = I2CMasterErr(base_to_use);
        //"close" variable argument list
        va_end(vargs);
    }
     
    //otherwise, we start transmission of multiple bytes on the
    //I2C bus
    else
    {
        //Initiate send of data from the MCU
        // repeated start is different than this only in the finishing byte-
        // "I2C_MASTER_CMD_BURST_SEND_FINISH" should be changed
        // Refer specifically to section 16.3.1.5 for repeated start stuff. 
        I2CMasterControl(base_to_use, I2C_MASTER_CMD_BURST_SEND_START);
         
        // Wait until MCU is done transferring.
        // "when the busy bit is 0" step 3 sec 16.3.1.5
        // Manual says to just keep checking so thats what we do. 
        while(I2CMasterBusy(base_to_use));
         
        //send num_of_args-2 pieces of data, using the
        //BURST_SEND_CONT command of the I2C module
        for(uint8_t i = 0; i < (num_of_args - 1); i++)
        {
            //put next piece of data into I2C FIFO
            I2CMasterDataPut(base_to_use, va_arg(vargs, uint32_t));
            //send next data that was just placed into FIFO
            // this "SEND_CONT" is 0x1 in the file: 
            ///.energia15/packages/Arduino-Tiva/hardware/tivac/1.0.2/system/driverlib/i2c.h
            // So use this for NOT repeated start and 
            // use 0x3 (refer to that .h file) for repeated start
            I2CMasterControl(base_to_use, I2C_MASTER_CMD_BURST_SEND_CONT);
			
            // Wait until MCU is done transferring.
            while(I2CMasterBusy(base_to_use));
			
			error_return = I2CMasterErr(base_to_use);
			if (error_return) return error_return;
        }
     
        //put last piece of data into I2C FIFO
        I2CMasterDataPut(base_to_use, va_arg(vargs, uint32_t));
        //send next data that was just placed into FIFO
        I2CMasterControl(base_to_use, I2C_MASTER_CMD_BURST_SEND_FINISH);
        // Wait until MCU is done transferring.
        while(I2CMasterBusy(base_to_use));
        error_return = I2CMasterErr(base_to_use);

        //"close" variable args list
        va_end(vargs);
    }

    return error_return;
}


// *****************
// Reads a variable number of bytes as a read to slave
// *****************
//sends an I2C command to the specified slave. See page 1001 of the microcontroller manual
uint32_t I2CReadNbytes(const int base,uint8_t slave_addr, uint8_t num_of_args, uint8_t reg, uint8_t * readStore)
{
    uint32_t error_return=0; // error defs comments at end of function
    uint32_t base_to_use;
	switch (base) {
		case 0: base_to_use=I2C0_BASE;
			break;
		case 1: base_to_use=I2C1_BASE;
			break;
		case 2: base_to_use=I2C2_BASE;
			break;
		case 3: base_to_use=I2C3_BASE;
			break;
		
		default: return (uint8_t) 69;
	}
	
	/************ Write the command/register ************************/
    I2CMasterSlaveAddrSet(base_to_use, slave_addr, false);
 
    //specify register to be read
    I2CMasterDataPut(base_to_use, reg);
 
    //send control byte and register address byte to slave device
    I2CMasterControl(base_to_use, I2C_MASTER_CMD_BURST_SEND_START);
     
    //wait for MCU to finish transaction
    while(I2CMasterBusy(base_to_use));
	
	error_return = I2CMasterErr(base_to_use);
	if (error_return) return error_return;
	
	/****************************************************************/
	/*********** Take a read ****************************************/   
    I2CMasterSlaveAddrSet(base_to_use, slave_addr, true);
    I2CMasterControl(base_to_use, I2C_MASTER_CMD_BURST_RECEIVE_START);
    while(I2CMasterBusy(base_to_use));
	
	LT_union_int32_4bytes * data = (LT_union_int32_4bytes *) readStore;
	
	uint8_t i = num_of_args -1;
         
    while (i>0) {
		I2CMasterControl(base_to_use, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
		
        data->LT_byte[i] = I2CMasterDataGet(base_to_use);

        while(I2CMasterBusy(base_to_use));
		
		error_return = I2CMasterErr(base_to_use);
		if (error_return) return error_return;
		
		i--;
    }
	
    data->LT_byte[0] = I2CMasterDataGet(base_to_use);
    while(I2CMasterBusy(base_to_use));
	
	I2CMasterControl(base_to_use, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
    // Wait until MCU is done transferring.
    while(I2CMasterBusy(base_to_use));
	
	if (num_of_args == 3) data->LT_uint32 = 0x0FFFFFF & data->LT_int32;
	else if (num_of_args == 2) {
		data->LT_2byte[0] = data->LT_2byte[0] >> 4;
	}
	
    error_return = I2CMasterErr(base_to_use);

    return error_return;
}

