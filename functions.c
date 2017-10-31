/* 
 *
 * A bunch of functions
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
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/mw/display/Display.h>
#include <ti/mw/display/DisplayExt.h>
#include <driverlib/aon_batmon.h>

/* Board Header files */
#include "Board.h"

/* Project Header files */
#include "sensors/bmp280.h"
#include "sensors/hdc1000.h"
#include "sensors/mpu9250.h"
#include "sensors/opt3001.h"
#include "sensors/tmp007.h"
#include "wireless/comm_lib.h"

#include "graphics.h"

/* Prototypes */
#include "functions.h"


Display_Handle hDisplay;


void intro(tContext *pContext) {
    
    uint8_t i=0, j=0;
    GrImageDraw(pContext, &opening, 0, 1);
    GrLineDraw(pContext, 5, 80, 91, 80);
    GrFlush(pContext);
    while (j<100){
        Task_sleep(350000 / Clock_tickPeriod);
        Display_doClearLines(hDisplay, 8, 9);
        GrImageDraw(pContext, &chaOP1, i, 71);
        if (i>25){
            j=j+3;
            GrImageDraw(pContext, &tank, j, 68);
        }
        GrFlush(pContext);
        i=i+3;
        Task_sleep(350000 / Clock_tickPeriod);
        Display_doClearLines(hDisplay, 8, 9);    
        GrImageDraw(pContext, &chaOP2, i, 71);
        if (i>25){
            j=j+3;
            GrImageDraw(pContext, &tank, j, 68);
        }
        GrFlush(pContext);
        i=i+3;
    }
}


void check_sensors(I2C_Handle *i2c_addr, I2C_Params *i2cParams_addr, uint8_t *slippery, uint8_t *other_tree, uint8_t *dark, uint8_t *cold) {
    
    I2C_Handle      i2c;
    I2C_Params      i2cParams;
    i2c = *i2c_addr;
    i2cParams = *i2cParams_addr;
    
    // OTHER SENSOR OPEN I2C
    i2c = I2C_open(Board_I2C0, &i2cParams);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }
    
    double pressure, temp0;
    bmp280_get_data(&i2c, &pressure, &temp0);
    double temp1, humidity;
    hdc1000_get_data(&i2c, &temp1, &humidity);
    double light;
    light = opt3001_get_data(&i2c);
    double temperature;
    temperature = tmp007_get_data(&i2c);
    
    // OTHER SENSOR CLOSE I2C
    I2C_close(i2c);
    
    char str[100];
    sprintf(str,"pressure: %.2f, humidity: %.2f, humtemp: %.2f, light: %.2f, temperature: %.2f\n",pressure,humidity,temp1,light,temperature);
    System_printf(str);
    System_flush();
    
    if (pressure < 109000) {
        *slippery = 1;
    }
    
    if (humidity < 30) {
        *other_tree = 1;
    }
    
    if (light < 20) {
        *dark = 1;
        Display_clear(hDisplay);
        Task_sleep(1000000 / Clock_tickPeriod);
        Display_print0(hDisplay, 1, 0, "   It's dark");
        Display_print0(hDisplay, 2, 0, "  out there...");
        Task_sleep(4000000 / Clock_tickPeriod);
        Display_print0(hDisplay, 4, 0, "Here, take this");
        Display_print0(hDisplay, 5, 0, "   flashlight");
        Display_print0(hDisplay, 6, 0, "   with you.");
        Task_sleep(4000000 / Clock_tickPeriod);
        Display_print0(hDisplay, 8, 0, "   You might");
        Display_print0(hDisplay, 9, 0, "   need it.");
        Task_sleep(7000000 / Clock_tickPeriod);
    }
    
    if (temperature < 25) {
        *cold = 1;
    }
    
    sprintf(str,"slippery: %d, other tree: %d, dark: %d, cold: %d\n",*slippery,*other_tree,*dark,*cold);
    System_printf(str);
    System_flush();
    
}


void update_rates(uint8_t slippery, float *left_rate, float *right_rate, float *up_rate, float *down_rate) {
    
    if (slippery == 1) {
        *left_rate = (*left_rate)*0.7;
        *right_rate = (*right_rate)*0.7;
        *up_rate = (*up_rate)*0.7;
        *down_rate = (*down_rate)*0.7;
    }
    
}


void print_rows(tContext *pContext) {
    pContext = DisplayExt_getGrlibContext(hDisplay);
    GrLineDraw(pContext,0,8,96,8);
    GrLineDraw(pContext,0,9,96,9);
    GrLineDraw(pContext,0,86,96,86);
    GrLineDraw(pContext,0,85,96,85);
    GrFlush(pContext);
}


void print_menu(tContext *pContext, uint8_t *i1, uint8_t *i2, uint8_t *i3, uint8_t *i4, uint8_t *i5, uint8_t *menurow, uint8_t *ledstate) {
    
    pContext = DisplayExt_getGrlibContext(hDisplay);
    
    uint32_t battery = AONBatMonBatteryVoltageGet();
    battery = (battery*125) >> 5;
    float voltage = ((float)battery)/1000;   
    char toprow[16];
    sprintf(toprow, "%.2f V", voltage);
    Display_print0(hDisplay, 0, 0, toprow);
    
    char menu4[16];
    if (*ledstate == 0) {
        sprintf(menu4, "Led: Off");
    }
    else if (*ledstate == 1) {
        sprintf(menu4, "Led: On");
    }
    
    if (*menurow == 5) {
        Display_print0(hDisplay, 2, *i2, "Help");
        Display_print0(hDisplay, 4, *i3, "Calibration");
        Display_print0(hDisplay, 6, *i4, menu4);
        Display_print0(hDisplay, 8, *i5, "Quit");
    }
    else if (*menurow != 5) { 
        Display_print0(hDisplay, 2, *i1, "Start Game");
        Display_print0(hDisplay, 4, *i2, "Help");
        Display_print0(hDisplay, 6, *i3, "Calibration");
        Display_print0(hDisplay, 8, *i4, menu4);
    }
    
}


void help(tContext *pContext) {
    Display_clear(hDisplay);
    print_rows(pContext);
    Display_print0(hDisplay, 0, 0, "Game Play:");
    Display_print0(hDisplay, 2, 0, "To Change Lane:");
    Display_print0(hDisplay, 3, 0, "Tilt Device <-->");
    Display_print0(hDisplay, 5, 0, "To Jump:"); 
    Display_print0(hDisplay, 6, 0, "Up And Down");
    Display_print0(hDisplay, 8, 0, "To See Better:");
    Display_print0(hDisplay, 9, 0, "Turn Back Led On");       
    Task_sleep(15000000 / Clock_tickPeriod);    
    Display_clear(hDisplay);
    print_rows(pContext);
}


void calib_start(void) {
    Display_clear(hDisplay);
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 0, 0, "Calibration");
    Task_sleep(1000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 2, 0, "- with");
    Display_print0(hDisplay, 3, 0, "  instructions:");
    Display_print0(hDisplay, 4, 0, "  upper button");
    Task_sleep(3000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 6, 0, "- without");
    Display_print0(hDisplay, 7, 0, "  instructions:");
    Display_print0(hDisplay, 8, 0, "  lower button");
    Task_sleep(3000000 / Clock_tickPeriod);
    Display_print0(hDisplay, 10, 0, "Your choice?");
}


//kentän piirto funktio
void field(tContext *pContext, uint8_t dark, uint8_t ledstate) {
    
    pContext = DisplayExt_getGrlibContext(hDisplay);

    // borders
    GrLineDraw(pContext,20,39,20,86);
    GrLineDraw(pContext,74,39,74,86);
    
    if (dark == 0 || ledstate == 1) {
        
        // borders
        GrLineDraw(pContext,20,8,20,86);
        GrLineDraw(pContext,74,8,74,86);
        
        //5s
        GrLineDraw(pContext,48,10,48,11);
        GrLineDraw(pContext,47,10,47,11);
        GrLineDraw(pContext,48,17,48,19);
        GrLineDraw(pContext,47,17,47,19);
        GrLineDraw(pContext,48,24,48,24);
        GrLineDraw(pContext,47,24,47,24);
        
        //4s
        GrLineDraw(pContext,48,25,48,26);
        GrLineDraw(pContext,47,25,47,26);
        GrLineDraw(pContext,48,32,48,34);
        GrLineDraw(pContext,47,32,47,34);
        GrLineDraw(pContext,48,39,48,39);
        GrLineDraw(pContext,47,39,47,39);
    }
    
    //3s
    GrLineDraw(pContext,48,40,48,41);
    GrLineDraw(pContext,47,40,47,41);
    GrLineDraw(pContext,48,47,48,49);
    GrLineDraw(pContext,47,47,47,49);
    GrLineDraw(pContext,48,54,48,54);
    GrLineDraw(pContext,47,54,47,54);
    
    //2s
    GrLineDraw(pContext,48,55,48,56);
    GrLineDraw(pContext,47,55,47,56);
    GrLineDraw(pContext,48,62,48,64);
    GrLineDraw(pContext,47,62,47,64);
    GrLineDraw(pContext,48,69,48,69);
    GrLineDraw(pContext,47,69,47,69);
    
    //1s
    GrLineDraw(pContext,48,70,48,71);
    GrLineDraw(pContext,47,70,47,71);
    GrLineDraw(pContext,48,77,48,79);
    GrLineDraw(pContext,47,77,47,79);
    GrLineDraw(pContext,48,84,48,84);
    GrLineDraw(pContext,47,84,47,84);       
    
    //hor
    GrLineDraw(pContext,0,86,96,86);
    GrLineDraw(pContext,0,85,96,85);
    GrLineDraw(pContext,0,8,96,8);        
    GrLineDraw(pContext,0,9,96,9);
    
    GrFlush(pContext);
    
}


void handle_msg(uint16_t *senderAddr, char *payload, uint8_t *row1, uint8_t *row2, uint8_t *row3, uint8_t *row4, uint8_t *row5, uint8_t dark, uint8_t ledstate) {
    
    char msg[16];
    char test[16];
    uint8_t row[4] = {0,0,0,0};
    uint8_t new_row;
    uint8_t mask = 0x01;
    
    if (*senderAddr == IEEE80154_SERVER_ADDR) {
        // Prints sender name and message (8 characters)
        // Lets the old message stay if new one is empty
        if (*(payload+1) != 0x20) {
            sprintf(msg,"%x: %.*s", *senderAddr, 8, payload+1);
            Display_print0(hDisplay, 11, 0, msg);
        
            System_printf(msg);
            System_flush();
            System_printf("\n");
            System_flush();
        }
    
        new_row = *payload;
        
        /*
        sprintf(test,"new row: %x\n", new_row);
        System_printf(test);
        System_flush();
        */
    
        //sprintf(test,"new row: %d,%d,%d,%d\n", *(row), *(row1+1), *(row1+2), *(row1+3));
        //System_printf(test);
        //System_flush();
    
    
        /* LEFT */
    
    
        // bit7 = 1 -> tree (3) on the left
        if (((new_row >> 7) & mask) == 1) {
            row[0] = 3;
        }
    
        // bit6 = 1
        // bit7 = 0 -> moving obstacle (2) on the left
        // bit7 = 1 -> moving obstacle and tree (4) on the left
        if (((new_row >> 6) & mask) == 1) {
            if (((new_row >> 7) & mask) == 0) {
                row[0] = 2;
            }
            if (((new_row >> 7) & mask) == 1) {
                row[0] = 4;
            }
        }
    
        // bit5 = 1 -> moving obstacle (2) on left lane
        if (((new_row >> 5) & mask) == 1) {
            row[1] = 2;
        }
    
        // bit4 = 1 -> static obstacle (1) on left lane
        if (((new_row >> 4) & mask) == 1) {
            row[1] = 1;
        }
    
    
        /* RIGHT */
    
    
        // bit0 = 1 -> tree (3) on the right
        if (((new_row >> 0) & mask) == 1) {
            row[3] = 3;
        }
    
        // bit1 = 1
        // bit0 = 0 -> moving obstacle (2) on the right
        // bit0 = 1 -> moving obstacle and tree (4) on the right
        if (((new_row >> 1) & mask) == 1) {
            if (((new_row >> 0) & mask) == 0) {
                row[3] = 2;
            }
            if (((new_row >> 0) & mask) == 1) {
                row[3] = 4;
            }
        }
    
        // bit2 = 1 -> moving obstacle (2) on right lane
        if (((new_row >> 2) & mask) == 1) {
            row[2] = 2;
        }
    
        // bit3 = 1 -> static obstacle (1) on right lane
        if (((new_row >> 3) & mask) == 1) {
            row[2] = 1;
        }
        
        uint8_t i;
        
        if (dark == 0 || ledstate == 1) {
            for (i=0; i<4; i++) {
                if (*(row1+i) == 2) {
                    *(row1+i) = 0;
                }
                else if (*(row1+i) == 4) {
                    *(row1+i) = 3;
                }
            }
        }
        
        else {
            for (i=0; i<4; i++) {
                if (*(row3+i) == 2) {
                    *(row3+i) = 0;
                }
                else if (*(row3+i) == 4) {
                    *(row3+i) = 3;
                }
            }
        }
    
        memcpy(row5, row4, 4);
        memcpy(row4, row3, 4);
        memcpy(row3, row2, 4);
        memcpy(row2, row1, 4);
        memcpy(row1, row, 4);
        
        /*
        sprintf(test,"row1: %d,%d,%d,%d\n", *(row1), *(row1+1), *(row1+2), *(row1+3));
        System_printf(test);
        System_flush();
        sprintf(test,"row2: %d,%d,%d,%d\n", *(row2), *(row2+1), *(row2+2), *(row2+3));
        System_printf(test);
        System_flush();
        sprintf(test,"row3: %d,%d,%d,%d\n", *(row3), *(row3+1), *(row3+2), *(row3+3));
        System_printf(test);
        System_flush();
        sprintf(test,"row4: %d,%d,%d,%d\n", *(row4), *(row4+1), *(row4+2), *(row4+3));
        System_printf(test);
        System_flush();
        sprintf(test,"row5: %d,%d,%d,%d\n", *(row5), *(row5+1), *(row5+2), *(row5+3));
        System_printf(test);
        System_flush();
        */
    }
    
    else {
        // Prints sender name and message (8 characters)
        // Lets the old message stay if new one is empty
        if (*(payload) != 0) {
            sprintf(msg,"%x: %.*s", *senderAddr, 9, payload);
            Display_print0(hDisplay, 11, 0, msg);
        
            System_printf(msg);
            System_flush();
            System_printf("\n");
            System_flush();
        }
    }
    
}


void update_road_row(tContext *pContext, uint8_t *row, uint8_t row_number, uint8_t other_tree) {
    
    pContext = DisplayExt_getGrlibContext(hDisplay);
    
    uint8_t i;
    
    // Empty old lanes
    GrImageDraw(pContext, &left_rev, 24, ((row_number-1)*15)+12);
    GrImageDraw(pContext, &right_rev, 56, ((row_number-1)*15)+12);
    
    // Empty old sides
    GrImageDraw(pContext, &left_side_rev, 0, ((row_number-1)*15)+12);
    GrImageDraw(pContext, &right_rev, 80, ((row_number-1)*15)+12);
    GrImageDraw(pContext, &left_side_rev, 0, ((row_number-1)*15)+12);
    GrImageDraw(pContext, &right_rev, 80, ((row_number-1)*15)+12);
    
    for (i=0; i<4; i++) {
        switch (*(row+i)) {
            case 1:
                GrImageDraw(pContext, &obs_stat, 30*i, ((row_number-1)*15)+12);
                break;
            case 2:
                GrImageDraw(pContext, &obs_mov, 30*i, ((row_number-1)*15)+12);
                break;
            case 3:
                if (other_tree == 0) {
                    GrImageDraw(pContext, &obs_tree_1, 30*i, ((row_number-1)*15)+12);
                }
                else {
                    GrImageDraw(pContext, &obs_tree_2, 30*i, ((row_number-1)*15)+12);
                }
                break;
            case 4:
                GrImageDraw(pContext, &obs_mov, 30*i, ((row_number-1)*15)+12);
                if (other_tree == 0) {
                    GrImageDraw(pContext, &obs_tree_1, 30*i, ((row_number-1)*15)+12);
                }
                else {
                    GrImageDraw(pContext, &obs_tree_2, 30*i, ((row_number-1)*15)+12);
                }
                break;
        }
    }
    
    GrFlush(pContext);

}


void update_road(tContext *pContext, uint8_t *row1, uint8_t *row2, uint8_t *row3, uint8_t *row4, uint8_t *row5, uint8_t other_tree, uint8_t dark, uint8_t ledstate) {
    
    pContext = DisplayExt_getGrlibContext(hDisplay);
    
    if (dark == 0 || ledstate == 1) {
        update_road_row(pContext, row1, 1, other_tree);
        update_road_row(pContext, row2, 2, other_tree);
    }
    update_road_row(pContext, row3, 3, other_tree);
    update_road_row(pContext, row4, 4, other_tree);
    update_road_row(pContext, row5, 5, other_tree);
    
}


void read_mpu(I2C_Handle *i2c_addr, I2C_Params *i2cParams_addr, float *left_rate, float *right_rate, float *up_rate, float *down_rate, uint8_t *playerRight, uint8_t *playerJump) {
    
    I2C_Handle      i2cMPU;
    I2C_Params      i2cMPUParams;
    i2cMPU = *i2c_addr;
    i2cMPUParams = *i2cParams_addr;
    
    // MPU OPEN I2C
	i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
	if (i2cMPU == NULL) {
	    System_abort("Error Initializing I2CMPU\n");
	}
	
	float ax, ay, az, gx, gy, gz;
	mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
	
	// MPU CLOSE I2C
	I2C_close(i2cMPU);
	
	if (*playerRight == 0 && *playerJump == 0 && gz > *right_rate) {
	    *playerRight = 1;
	}
	
	else if (*playerRight == 1 && *playerJump == 0 && gz < *left_rate) {
	    *playerRight = 0;
    }
    
    else if (*playerJump == 0 && gx < *up_rate) {
        *playerJump = 1;
    }
    
    else if (*playerJump == 1 && gx > *down_rate) {
        *playerJump = 0;
    }
    
}


void update_player(tContext *pContext, uint8_t *playerRight, uint8_t *playerJump, uint8_t *cold, uint8_t *row5, uint8_t *other_tree) {
    
    pContext = DisplayExt_getGrlibContext(hDisplay);
    
    static uint8_t number = 0;
    
    update_road_row(pContext, row5, 5, other_tree);
    
    if (*cold == 0) {
        if (*playerRight == 0 && *playerJump == 0) {
            if ((number % 4) == 0 || (number % 4) == 1) {
                GrImageDraw(pContext, &left_rev, 24, 70);
                GrImageDraw(pContext, &right_rev, 56, 70);
                GrImageDraw(pContext, &char_anim_1, 37, 70);
            }
            else {
                GrImageDraw(pContext, &left_rev, 24, 70);
                GrImageDraw(pContext, &right_rev, 56, 70);
                GrImageDraw(pContext, &char_anim_2, 37, 70);
            }
        }
    
        else if (*playerRight == 0 && *playerJump == 1) {
            GrImageDraw(pContext, &left_rev, 24, 70);
            GrImageDraw(pContext, &char_jump, 30, 70);
        }
    
        else if (*playerRight == 1 && *playerJump == 0) {
            if ((number % 4) == 0 || (number % 4) == 1) {
                GrImageDraw(pContext, &left_rev, 24, 70);
                GrImageDraw(pContext, &right_rev, 56, 70);
                GrImageDraw(pContext, &char_anim_1, 59, 70);
            }
            else {
                GrImageDraw(pContext, &left_rev, 24, 70);
                GrImageDraw(pContext, &right_rev, 56, 70);
                GrImageDraw(pContext, &char_anim_2, 59, 70);
            }
        }
    
        else if (*playerRight == 1 && *playerJump == 1) {
            GrImageDraw(pContext, &right_rev, 56, 70);
            GrImageDraw(pContext, &char_jump, 60, 70);
        }
    }
    
    else {
        if (*playerRight == 0 && *playerJump == 0) {
            if (number % 2 == 0) {
                GrImageDraw(pContext, &left_rev, 24, 70);
                GrImageDraw(pContext, &right_rev, 56, 70);
                GrImageDraw(pContext, &char_anim_1, 37, 70);
            }
            else {
                GrImageDraw(pContext, &left_rev, 24, 70);
                GrImageDraw(pContext, &right_rev, 56, 70);
                GrImageDraw(pContext, &char_anim_2, 37, 70);
            }
        }
    
        else if (*playerRight == 0 && *playerJump == 1) {
            GrImageDraw(pContext, &left_rev, 24, 70);
            GrImageDraw(pContext, &char_jump, 30, 70);
        }
    
        else if (*playerRight == 1 && *playerJump == 0) {
            if (number % 2 == 0) {
                GrImageDraw(pContext, &left_rev, 24, 70);
                GrImageDraw(pContext, &right_rev, 56, 70);
                GrImageDraw(pContext, &char_anim_1, 59, 70);
            }
            else {
                GrImageDraw(pContext, &left_rev, 24, 70);
                GrImageDraw(pContext, &right_rev, 56, 70);
                GrImageDraw(pContext, &char_anim_2, 59, 70);
            }
        }
    
        else if (*playerRight == 1 && *playerJump == 1) {
            GrImageDraw(pContext, &right_rev, 56, 70);
            GrImageDraw(pContext, &char_jump, 60, 70);
        }
    }
    
    GrFlush(pContext);
    number++;
    
}


void check_if_lost(uint8_t *lost, uint8_t *row5, uint8_t *playerRight, uint8_t *playerJump) {
    
    if (*playerRight == 0 && *playerJump == 0 && *(row5+1) != 0) {
        *lost = 1;
        System_printf("You lost!\n");
        System_flush();
    }
    
    else if (*playerRight == 1 && *playerJump == 0 && *(row5+2) != 0) {
        *lost = 1;
        System_printf("You lost!\n");
        System_flush();
    }
    
    else {
        *lost = 0;
    }
    
}


void show_level(uint16_t level) {
    
    uint32_t battery = AONBatMonBatteryVoltageGet();
    battery = (battery*125) >> 5;
    float voltage = ((float)battery)/1000;   
    
    uint16_t lvl = level;
    
    if (level > 999) {
        lvl = 999;
    }
    
    char toprow[16];
    sprintf(toprow, "%.2f V       %d", voltage, lvl);
    Display_print0(hDisplay, 0, 0, toprow);
    
}


void game_over(tContext *pContext, uint16_t level) {
    
    pContext = DisplayExt_getGrlibContext(hDisplay);
    char str[16];
    
    Display_clear(hDisplay);
    GrImageDraw(pContext, &ending,0,0);
    GrFlush(pContext);
    Task_sleep(4000000/Clock_tickPeriod);
    GrImageDraw(pContext, &ending_rev,31,62);
    GrFlush(pContext);
    Display_print0(hDisplay, 9, 3, "Your score:");
    sprintf(str, "%d", level);
    Display_print0(hDisplay, 10, 7, str);
    Task_sleep(4000000/Clock_tickPeriod);
    Display_clear(hDisplay);
}


