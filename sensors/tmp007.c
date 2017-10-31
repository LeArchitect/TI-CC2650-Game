/*
 * tmp007.c
 *
 *  Created on: 28.9.2016
 *  Author: Teemu Leppanen / UBIComp / University of Oulu
 *
 *  Datakirja: http://www.ti.com/lit/ds/symlink/tmp007.pdf
 */

#include <xdc/runtime/System.h>
#include <string.h>
#include "Board.h"
#include "tmp007.h"

I2C_Transaction i2cTransaction;
char txBuffer[4];
char rxBuffer[2];

void tmp007_setup(I2C_Handle *i2c) {

	System_printf("TMP007: Config not needed!\n");
    System_flush();
}

double tmp007_get_data(I2C_Handle *i2c) {

	double temperature = 0.0;

	// FILL OUT THIS DATA STRUCTURE TO GET TEMPERATURE DATA
	txBuffer[0] = TMP007_REG_TEMP;
    i2cTransaction.slaveAddress = Board_TMP007_ADDR;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 2;

	if (I2C_transfer(*i2c, &i2cTransaction)) {

		// JTKJ: HERE YOU GET THE TEMPERATURE VALUE FROM RXBUFFER AS BYTES
		
		uint16_t result = 0;
		result = (result | rxBuffer[0]) << 8;
		result = (result | rxBuffer[1]) >> 2;
		
		temperature = result*0.03125;

	} else {

		System_printf("TMP007: Data read failed!\n");
		System_flush();
	}

	return temperature;
}


