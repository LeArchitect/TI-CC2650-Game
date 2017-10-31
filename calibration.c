/* 
 *
 * Functions to calibrate the mpu sensor
 *
 */


/* XDCtools Header files */
// #include <xdc/std.h>
#include <xdc/runtime/System.h>
// #include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

/* TI-RTOS Header files */
#include <ti/drivers/I2C.h>
// #include <ti/drivers/PIN.h>
// #include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/mw/display/Display.h>
// #include <ti/mw/display/DisplayExt.h>

/* Board Header files */
#include "Board.h"

/* Project Header files */
#include "sensors/mpu9250.h"

/* Prototypes */
#include "calibration.h"


Display_Handle hDisplay;


void minmaxmean(float *array, uint8_t size, float *min, float *max, float *mean) {
    
    // calculates the minimum, maximum and mean of an array of floats

    float maximum, minimum, sum;
    maximum = *array;
    minimum = *array;
    sum = *array; 
    uint8_t i;

    for (i=1; i<size; i++) {
        
        if (*(array+i) < minimum) {
            minimum = *(array+i);
        }

        if (*(array+i) > maximum) {
            maximum = *(array+i);
        }
        
        sum += *(array+i);
    }
    
    *min = minimum;
    *max = maximum;
    *mean = sum/size;
    
}


void instructions1() {
    
    // prints the first set of instructions on the device screen
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Let us calibrate");
    Display_print0(hDisplay, 1, 0, "the gyroscope");
    Display_print0(hDisplay, 2, 0, "sensor of this");
    Display_print0(hDisplay, 3, 0, "device to suit");
    Display_print0(hDisplay, 4, 0, "your personal");
    Display_print0(hDisplay, 5, 0, "playing style.");
    Task_sleep(7000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 7, 0, "You will be");
    Display_print0(hDisplay, 8, 0, "asked to hold");
    Display_print0(hDisplay, 9, 0, "your device and");
    Display_print0(hDisplay, 10, 0, "then tilt it in");
    Display_print0(hDisplay, 11, 0, "different ways.");
    Task_sleep(10000000 / Clock_tickPeriod);
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "First of all,");
    Display_print0(hDisplay, 1, 0, "just hold your");
    Display_print0(hDisplay, 2, 0, "device in your");
    Display_print0(hDisplay, 3, 0, "hands for a few");
    Display_print0(hDisplay, 4, 0, "seconds.");
    Task_sleep(7000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 6, 0, "Try not to move");
    Display_print0(hDisplay, 7, 0, "it too much");
    Display_print0(hDisplay, 8, 0, "during this");
    Display_print0(hDisplay, 9, 0, "part of the");
    Display_print0(hDisplay, 10, 0, "calibration.");
    Task_sleep(10000000 / Clock_tickPeriod);
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Are you ready?");
    Display_print0(hDisplay, 1, 0, "Please keep your");
    Display_print0(hDisplay, 2, 0, "device rather");
    Display_print0(hDisplay, 3, 0, "still for now.");
    Task_sleep(7000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 5, 0, "Here we go...");
    Task_sleep(5000000 / Clock_tickPeriod);
    
}


void rest_part(I2C_Handle *i2c_addr, I2C_Params *i2cParams_addr, float *rest_gz_min_addr, float *rest_gz_max_addr, float *rest_gx_min_addr, float *rest_gx_max_addr) {
    
    // rest part
    
    I2C_Handle      i2cMPU;
    I2C_Params      i2cMPUParams;
    i2cMPU = *i2c_addr;
    i2cMPUParams = *i2cParams_addr;
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Please wait...");
    
    // MPU OPEN I2C
	i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
	if (i2cMPU == NULL) {
	    System_abort("Error Initializing I2CMPU\n");
	}
    
    float ax, ay, az, gx, gy, gz;
    float rest_gz[10], rest_gx[10];
    char str[30];
    System_printf("Rest values:\n");
    uint8_t i;
    
    for (i=0; i<10; i++) {
        mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
    	rest_gz[i] = gz;
    	rest_gx[i] = gx;
        Display_print0(hDisplay, i+1, 0, "...");
        sprintf(str,"L/R: %.2f, U/D: %.2f\n", gz, gx);
        System_printf(str);
	    System_flush();
        Task_sleep(500000 / Clock_tickPeriod);
    }
    
    // MPU CLOSE I2C
	I2C_close(i2cMPU);
    
    Display_print0(hDisplay, 11, 0, "Done!");
    
	float rest_gz_mean, rest_gx_mean;
	minmaxmean(&rest_gz[0], 10, rest_gz_min_addr, rest_gz_max_addr, &rest_gz_mean);
	minmaxmean(&rest_gx[0], 10, rest_gx_min_addr, rest_gx_max_addr, &rest_gx_mean);
	
    System_printf("L/R:\n");
    sprintf(str,"min: %.2f\n", *rest_gz_min_addr);
    System_printf(str);
    sprintf(str,"max: %.2f\n", *rest_gz_max_addr);
    System_printf(str);
    sprintf(str,"mean: %.2f\n", rest_gz_mean);
    System_printf(str);
    System_flush();
    
    System_printf("U/D:\n");
    sprintf(str,"min: %.2f\n", *rest_gx_min_addr);
    System_printf(str);
    sprintf(str,"max: %.2f\n", *rest_gx_max_addr);
    System_printf(str);
    sprintf(str,"mean: %.2f\n", rest_gx_mean);
    System_printf(str);
    System_flush();
    
}


void instructions2() {
    
    // instructions for rotating to the left
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Soon, you will");
    Display_print0(hDisplay, 1, 0, "be asked to tilt");
    Display_print0(hDisplay, 2, 0, "your device to");
    Display_print0(hDisplay, 3, 0, "the LEFT.");
    Task_sleep(5000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 5, 0, "(Anti-clockwise)");
    Task_sleep(5000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 7, 0, "Take a look at");
    Display_print0(hDisplay, 8, 0, "this example...");
    Task_sleep(5000000 / Clock_tickPeriod);
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Your device now:");
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 1, 0, "      ---");
    Display_print0(hDisplay, 2, 0, "     |   |");
    Display_print0(hDisplay, 3, 0, "     |   |");
    Display_print0(hDisplay, 4, 0, "      ---");
    Task_sleep(3000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 6, 0, "Tilted left:");
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 7, 0, "    ---");
    Display_print0(hDisplay, 8, 0, "    \\   \\");
    Display_print0(hDisplay, 9, 0, "     \\   \\");
    Display_print0(hDisplay, 10, 0, "       ---");
    Task_sleep(5000000 / Clock_tickPeriod);
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Rotating it ~45");
    Display_print0(hDisplay, 1, 0, "degrees will do.");
    Task_sleep(5000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 3, 0, "The faster you");
    Display_print0(hDisplay, 4, 0, "do it now, the");
    Display_print0(hDisplay, 5, 0, "faster you will");
    Display_print0(hDisplay, 6, 0, "have to do it in");
    Display_print0(hDisplay, 7, 0, "the game when");
    Display_print0(hDisplay, 8, 0, "you move to the");
    Display_print0(hDisplay, 9, 0, "left.");
    Task_sleep(10000000 / Clock_tickPeriod);
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Please turn the");
    Display_print0(hDisplay, 1, 0, "device when I");
    Display_print0(hDisplay, 2, 0, "say \"NOW!\".");
    Task_sleep(5000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 4, 0, "Like this:");
    Task_sleep(5000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 6, 0, "Rotate to the");
    Display_print0(hDisplay, 7, 0, "LEFT in");
    Task_sleep(2000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 8, 0, "3");
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 9, 0, "2");
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 10, 0, "1");
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 11, 0, "NOW!");
    Task_sleep(5000000 / Clock_tickPeriod);
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Are you ready?");
    Task_sleep(3000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 2, 0, "Here we go...");
    Task_sleep(5000000 / Clock_tickPeriod);
    
}


void instructions3() {
    
    // instructions for rotating to the right
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Soon, you will");
    Display_print0(hDisplay, 1, 0, "be asked to turn");
    Display_print0(hDisplay, 2, 0, "your device to");
    Display_print0(hDisplay, 3, 0, "the RIGHT.");
    Task_sleep(5000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 5, 0, "(Clockwise)");
    Task_sleep(3000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 7, 0, "The same thing");
    Display_print0(hDisplay, 8, 0, "as before,");
    Display_print0(hDisplay, 9, 0, "just different");
    Display_print0(hDisplay, 10, 0, "direction.");
    Task_sleep(7000000 / Clock_tickPeriod);
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Are you ready?");
    Task_sleep(3000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 2, 0, "Here we go...");
    Task_sleep(5000000 / Clock_tickPeriod);
    
}


void instructions4() {
    
    // instructions for rotating upwards
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Well done!");
    Display_print0(hDisplay, 1, 0, "Now all that");
    Display_print0(hDisplay, 2, 0, "remains is to");
    Display_print0(hDisplay, 3, 0, "tilt your device");
    Display_print0(hDisplay, 4, 0, "up and down.");
    Task_sleep(7000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 6, 0, "After that, you");
    Display_print0(hDisplay, 7, 0, "can finally play");
    Display_print0(hDisplay, 8, 0, "the game...");
    Task_sleep(7000000 / Clock_tickPeriod);
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "First, you will");
    Display_print0(hDisplay, 1, 0, "be asked to");
    Display_print0(hDisplay, 2, 0, "rotate this");
    Display_print0(hDisplay, 3, 0, "device upwards.");
    Task_sleep(7000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 5, 0, "That is, rotate");
    Display_print0(hDisplay, 6, 0, "it around the");
    Display_print0(hDisplay, 7, 0, "horizontal axis");
    Display_print0(hDisplay, 8, 0, "so that the top");
    Display_print0(hDisplay, 9, 0, "of the device");
    Display_print0(hDisplay, 10, 0, "goes towards");
    Display_print0(hDisplay, 11, 0, "you.");
    Task_sleep(10000000 / Clock_tickPeriod);
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 2, 0, "      ----");
    Display_print0(hDisplay, 3, 0, "     |    |");
    Display_print0(hDisplay, 4, 0, "  ------------");
    Display_print0(hDisplay, 5, 0, "     |    |");
    Display_print0(hDisplay, 6, 0, "      ----");
    Task_sleep(5000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Towards you");
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 8, 0, "Away from you");
    Task_sleep(5000000 / Clock_tickPeriod);
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Are you ready?");
    Task_sleep(3000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 2, 0, "Here we go...");
    Task_sleep(5000000 / Clock_tickPeriod);
    
}


void instructions5() {
    
    // instructions for rotating downwards
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Soon, you will");
    Display_print0(hDisplay, 1, 0, "be asked to");
    Display_print0(hDisplay, 2, 0, "rotate this");
    Display_print0(hDisplay, 3, 0, "device downwards");
    Task_sleep(7000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 4, 0, "so that the top");
    Display_print0(hDisplay, 5, 0, "of the device");
    Display_print0(hDisplay, 6, 0, "goes away from");
    Display_print0(hDisplay, 7, 0, "you and the");
    Display_print0(hDisplay, 8, 0, "bottom towards");
    Display_print0(hDisplay, 9, 0, "you.");
    Task_sleep(10000000 / Clock_tickPeriod);
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Are you ready?");
    Task_sleep(3000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 2, 0, "Here we go...");
    Task_sleep(5000000 / Clock_tickPeriod);
    
}


void move_part(I2C_Handle *i2c_addr, I2C_Params *i2cParams_addr, float *move_rate_addr, uint8_t direction) {
    
    // move part
    
    I2C_Handle      i2cMPU;
    I2C_Params      i2cMPUParams;
    i2cMPU = *i2c_addr;
    i2cMPUParams = *i2cParams_addr;
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    
    switch(direction) {
        case 1:
            Display_print0(hDisplay, 0, 0, "Rotate to the");
            Display_print0(hDisplay, 1, 0, "LEFT in");
            break;
        case 2:
            Display_print0(hDisplay, 0, 0, "Rotate to the");
            Display_print0(hDisplay, 1, 0, "RIGHT in");
            break;
        case 3:
            Display_print0(hDisplay, 0, 0, "Rotate UPWARDS");
            Display_print0(hDisplay, 1, 0, "in");
            break;
        case 4:
            Display_print0(hDisplay, 0, 0, "Rotate DOWNWARDS");
            Display_print0(hDisplay, 1, 0, "in");
            break;
    }
    
    Task_sleep(2000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 2, 0, "3");
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 3, 0, "2");
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 4, 0, "1");
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 5, 0, "NOW!");
    
    // MPU OPEN I2C
	i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
	if (i2cMPU == NULL) {
	    System_abort("Error Initializing I2CMPU\n");
	}
    
    uint8_t i;
    float values[10];
    float ax, ay, az, gx, gy, gz;
    
    if ((direction == 1) | (direction == 2)) {
        for (i=0; i<10; i++) {
            mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
    	    values[i] = gz;
    	    Task_sleep(40000 / Clock_tickPeriod);
        }
    }
    
    if ((direction == 3) | (direction == 4)) {
        for (i=0; i<10; i++) {
            mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
    	    values[i] = gx;
    	    Task_sleep(40000 / Clock_tickPeriod);
        }
    }
    
    // MPU CLOSE I2C
	I2C_close(i2cMPU);
    
    Display_print0(hDisplay, 7, 0, "Done!");
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 8, 0, "Thanks!");
    Task_sleep(2000000 / Clock_tickPeriod);
    
    char str[30];
    System_printf("Turn values:\n");
    
    for (i=0; i<10; i++) {
        sprintf(str,"%.2f\n", values[i]);
        System_printf(str);
    }
	
	System_flush();
	
	float values_min, values_max, values_mean;
	minmaxmean(&values[0], 10, &values_min, &values_max, &values_mean);
	
	sprintf(str,"min: %.2f\n", values_min);
    System_printf(str);
    sprintf(str,"max: %.2f\n", values_max);
    System_printf(str);
    sprintf(str,"mean: %.2f\n", values_mean);
    System_printf(str);
    System_flush();
	
	*move_rate_addr = values_mean;
    
}


void oops(uint8_t direction) {
    
    // something went wrong...
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Oops, it looks");
    Display_print0(hDisplay, 1, 0, "like something");
    Display_print0(hDisplay, 2, 0, "went wrong here.");
    Task_sleep(5000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 4, 0, "Maybe you didn't");
    Display_print0(hDisplay, 5, 0, "move the device");
    Display_print0(hDisplay, 6, 0, "in time, or");
    Display_print0(hDisplay, 7, 0, "maybe you turned");
    Display_print0(hDisplay, 8, 0, "it in the wrong");
    Display_print0(hDisplay, 9, 0, "direction.");
    Task_sleep(10000000 / Clock_tickPeriod);
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Let's try again.");
    
    switch (direction) {
        case 1:
            Display_print0(hDisplay, 1, 0, "Please turn your");
            Display_print0(hDisplay, 2, 0, "device to the");
            Display_print0(hDisplay, 3, 0, "LEFT when I say");
            Display_print0(hDisplay, 4, 0, "\"NOW!\".");
            break;
        case 2:
            Display_print0(hDisplay, 1, 0, "Please turn your");
            Display_print0(hDisplay, 2, 0, "device to the");
            Display_print0(hDisplay, 3, 0, "RIGHT when I say");
            Display_print0(hDisplay, 4, 0, "\"NOW!\".");
            break;
        case 3:
            Display_print0(hDisplay, 1, 0, "Please rotate");
            Display_print0(hDisplay, 2, 0, "your device");
            Display_print0(hDisplay, 3, 0, "UPWARDS when I");
            Display_print0(hDisplay, 4, 0, "say \"NOW!\".");
            break;
        case 4:
            Display_print0(hDisplay, 1, 0, "Please rotate");
            Display_print0(hDisplay, 2, 0, "your device");
            Display_print0(hDisplay, 3, 0, "DOWNWARDS when I");
            Display_print0(hDisplay, 4, 0, "say \"NOW!\".");
            break;
    }
    
    Task_sleep(7000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 6, 0, "Get ready!");
    Task_sleep(5000000 / Clock_tickPeriod);
        
}


void calibrate(I2C_Handle *i2c_addr, I2C_Params *i2cParams_addr, float *left_rate, float *right_rate, float *up_rate, float *down_rate, uint8_t help) {
    
    // calibrates mpu
    
    I2C_Handle      i2cMPU;
    I2C_Params      i2cMPUParams;
    i2cMPU = *i2c_addr;
    i2cMPUParams = *i2cParams_addr;
    
    switch (help) {
        case 0:
            Display_clear(hDisplay);
            Task_sleep(1000000 / Clock_tickPeriod);
            Display_print0(hDisplay, 0, 0, "First, hold your");
            Display_print0(hDisplay, 1, 0, "device still...");
            Task_sleep(3000000 / Clock_tickPeriod);
            break;
            
        case 1:
            instructions1();
            break;
    }
    
    float rest_gz_min, rest_gz_max, rest_gx_min, rest_gx_max;
    rest_part(&i2cMPU, &i2cMPUParams, &rest_gz_min, &rest_gz_max, &rest_gx_min, &rest_gx_max);
	
    switch (help) {
        case 0:
            Display_clear(hDisplay);
            Task_sleep(1000000 / Clock_tickPeriod);
            Display_print0(hDisplay, 0, 0, "Next, rotate to");
            Display_print0(hDisplay, 1, 0, "the left...");
            Task_sleep(3000000 / Clock_tickPeriod);
            break;
            
        case 1:
            instructions2();
            break;
    }
    
    move_part(&i2cMPU, &i2cMPUParams, left_rate, 1);
    
    while (*left_rate > rest_gz_min) {
        oops(1);
        move_part(&i2cMPU, &i2cMPUParams, left_rate, 1);
    }
    
    char str[30];
    sprintf(str,"left_rate: %.2f\n", *left_rate);
    System_printf(str);
    sprintf(str,"rest_gz_min: %.2f\n", rest_gz_min);
    System_printf(str);
    System_flush();
    
    switch (help) {
        case 0:
            Display_clear(hDisplay);
            Task_sleep(1000000 / Clock_tickPeriod);
            Display_print0(hDisplay, 0, 0, "Next, rotate to");
            Display_print0(hDisplay, 1, 0, "the right...");
            Task_sleep(3000000 / Clock_tickPeriod);
            break;
            
        case 1:
            instructions3();
            break;
    }
    
    move_part(&i2cMPU, &i2cMPUParams, right_rate, 2);
    
    while (*right_rate < rest_gz_max) {
        oops(2);
        move_part(&i2cMPU, &i2cMPUParams, right_rate, 2);
    }
    
    sprintf(str,"right_rate: %.2f\n", *right_rate);
    System_printf(str);
    sprintf(str,"rest_gz_max: %.2f\n", rest_gz_max);
    System_printf(str);
    System_flush();
    
    switch (help) {
        case 0:
            Display_clear(hDisplay);
            Task_sleep(1000000 / Clock_tickPeriod);
            Display_print0(hDisplay, 0, 0, "Next, rotate");
            Display_print0(hDisplay, 1, 0, "upwards...");
            Task_sleep(3000000 / Clock_tickPeriod);
            break;
            
        case 1:
            instructions4();
            break;
    }
    
    move_part(&i2cMPU, &i2cMPUParams, up_rate, 3);
    
    while (*up_rate > rest_gx_min) {
        oops(3);
        move_part(&i2cMPU, &i2cMPUParams, up_rate, 3);
    }
    
    sprintf(str,"up_rate: %.2f\n", *up_rate);
    System_printf(str);
    sprintf(str,"rest_gx_min: %.2f\n", rest_gx_min);
    System_printf(str);
    System_flush();
    
    switch (help) {
        case 0:
            Display_clear(hDisplay);
            Task_sleep(1000000 / Clock_tickPeriod);
            Display_print0(hDisplay, 0, 0, "Next, rotate");
            Display_print0(hDisplay, 1, 0, "downwards...");
            Task_sleep(3000000 / Clock_tickPeriod);
            break;
            
        case 1:
            instructions5();
            break;
    }
    
    move_part(&i2cMPU, &i2cMPUParams, down_rate, 4);
    
    while (*down_rate < rest_gx_max) {
        oops(4);
        move_part(&i2cMPU, &i2cMPUParams, down_rate, 4);
    }
    
    sprintf(str,"down_rate: %.2f\n", *down_rate);
    System_printf(str);
    sprintf(str,"rest_gx_max: %.2f\n", rest_gx_max);
    System_printf(str);
    System_flush();
    
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Calibration is");
    Display_print0(hDisplay, 1, 0, "finished!");
    Task_sleep(5000000 / Clock_tickPeriod);
    Display_clear(hDisplay);
    
}

