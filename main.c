/*
 *  ======== main.c ========
 *
 *  Authors: 
 *
 */

/* XDCtools Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/System.h>
#include <xdc/std.h>

/* TI-RTOS Header files */
#include <driverlib/aon_batmon.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/mw/display/Display.h>
#include <ti/mw/display/DisplayExt.h>

/* Board Header files */
#include "Board.h"

/* Project Header files */
#include "sensors/bmp280.h"
#include "sensors/hdc1000.h"
#include "sensors/mpu9250.h"
#include "sensors/opt3001.h"
#include "sensors/tmp007.h"
#include "wireless/comm_lib.h"

#include "calibration.h"
#include "functions.h"
#include "graphics.h"

/* Function prototypes */
void game(I2C_Handle *i2c_addr, I2C_Params *i2cParams_addr);
void messages(void);

/* Task Stacks */
#define STACKSIZE 2048
Char mainTaskStack[STACKSIZE];
Char commTaskStack[STACKSIZE];

/* Global variables */

// States
enum mainStates { MENU=1, GAME };
enum menuStates { PICK=1, CALIB, OTHER };
enum gameStates { READY=1, READ_SENSOR, MSG_WAITING, MSG_PICK, MSG_SEND };

enum mainStates mainState = MENU;
enum menuStates menuState = OTHER; // no buttons/sensors
enum gameStates gameState = READY;

// Menu variables
uint8_t i1=3, i2=1, i3=1, i4=1, i5=1;
uint8_t menurow=1;
uint8_t menuselect=0;
uint8_t calib_help=2;
uint8_t ledstate=0;

// Game variables
uint8_t lost=0;
uint16_t level=0;
uint8_t messagenum=1;
uint8_t playerRight=0;
uint8_t playerJump=0;
uint8_t slippery=0, other_tree=0, dark=0, cold=0;
uint8_t row1[4]={0,0,0,0}, row2[4]={0,0,0,0}, row3[4]={0,0,0,0}, row4[4]={0,0,0,0}, row5[4]={0,0,0,0};
float left_rate=-30, right_rate=30, up_rate=-30, down_rate=30;

// Strings and messages
// char test[32];
char str[16];
char payload[16];
char msg[16];
uint16_t senderAddr;


/* Display */
Display_Handle hDisplay;
tContext *pContext;

/* Mpu */
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};


/* Button0 configured as input */
static PIN_Handle hButton0;
static PIN_State sButton0;
PIN_Config cButton0[] = {
    Board_BUTTON0 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};


/* Button1 configured as power button */
static PIN_Handle hPowerButton;
static PIN_State sPowerButton;
PIN_Config cPowerButton[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};
PIN_Config cPowerWake[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PINCC26XX_WAKEUP_NEGEDGE,
    PIN_TERMINATE
};


/* Leds */
static PIN_Handle hLed;
static PIN_State sLed;
PIN_Config cLed[] = {
    Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};


/* Handler for Button0 */
Void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
    
    if (mainState == MENU && menuState == PICK) {
        if (menurow == 5) {
            menurow = 0;
    	}
    	menurow++;
    }
    
    else if (mainState == MENU && menuState == CALIB) {
        calib_help = 1;
    }
    
    else if (mainState == GAME && gameState == READY) {
        if (messagenum == 7) {
    	    messagenum = 1;
    	}
    	else if (messagenum > 7) {
    	    messagenum = 7;
    	}
        else {
            messagenum++;
        }
        gameState = MSG_PICK;
    }
}


/* Handler for power button */
Void powerButtonFxn(PIN_Handle handle, PIN_Id pinId) {
    
    if (mainState == MENU && menuState == PICK) {
        if (menuselect == 0) {
            menuselect = 1;
        }
    }
    
    else if (mainState == MENU && menuState == CALIB) {
        calib_help = 0;
    }
    
    else if (mainState == GAME && gameState == READY) {
        if (messagenum < 7) {
            gameState = MSG_SEND;
        }
    }
}


/* Interruption for sensor reading */
Void clkFxn(UArg arg0) {

    if (mainState == GAME && gameState == READY) {
        gameState = READ_SENSOR;
    }
    
}


/* Communication task */
Void commTask(UArg arg0, UArg arg1) {
    
    // Radio to receive mode
    int32_t result = StartReceive6LoWPAN();
    if(result != true) {
        System_abort("Wireless receive mode failed");
    }

    while (1) {
        
        if (GetRXFlag() == true && mainState == GAME && gameState == READY) {
            gameState = MSG_WAITING;
        }
        
        // TODO: optimize
        Task_sleep(1000/Clock_tickPeriod);
    }
}


/* Main task */
Void mainTask(UArg arg0, UArg arg1) {

    /* Init Display */
    Display_Params displayParams;
	displayParams.lineClearMode = DISPLAY_CLEAR_BOTH;
    Display_Params_init(&displayParams);

    hDisplay = Display_open(Display_Type_LCD, &displayParams);
    tContext *pContext = DisplayExt_getGrlibContext(hDisplay);
    if (hDisplay == NULL) {
        System_abort("Error initializing Display\n");
    }
    
    // Intro
    intro(pContext);
    
    game_over(pContext, level);
    
    /* i2c for other sensors */
    I2C_Handle      i2c;
    I2C_Params      i2cParams;
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    
    /* i2c for mpu */
    I2C_Handle      i2cMPU;
    I2C_Params      i2cMPUParams;
    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;
    
    // MPU OPEN I2C
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL) {
        System_abort("Error Initializing I2C for mpu\n");
    }
    
    // MPU POWER ON
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);

    // WAIT 100MS FOR THE SENSOR TO POWER UP
	Task_sleep(100000 / Clock_tickPeriod);
    System_printf("MPU9250: Power ON\n");
    System_flush();
    
    // MPU9250 SETUP AND CALIBRATION
    System_printf("MPU9250: Setup and calibration...\n");
	System_flush();

	mpu9250_setup(&i2cMPU);

	System_printf("MPU9250: Setup and calibration OK\n");
	System_flush();
	
	// MPU CLOSE I2C
    I2C_close(i2cMPU);
    
    // OTHER SENSOR OPEN I2C
    i2c = I2C_open(Board_I2C0, &i2cParams);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }
    
    // SENSOR SETUP
    bmp280_setup(&i2c);
    hdc1000_setup(&i2c);
    opt3001_setup(&i2c);
    tmp007_setup(&i2c);
    
    // OTHER SENSOR CLOSE I2C
    I2C_close(i2c);
    
    // Check sensor values
    
    check_sensors(&i2c, &i2cParams, &slippery, &other_tree, &dark, &cold);
    update_rates(slippery, &left_rate, &right_rate, &up_rate, &down_rate);
    
    Display_clear(hDisplay);

    ledstate = PIN_getOutputValue(Board_LED0);
    
    // Main loop
    while (1) {
        
        switch (mainState) {
            
            case MENU:
                print_rows(pContext);
                print_menu(pContext, &i1, &i2, &i3, &i4, &i5, &menurow, &ledstate);
                
                menuState = PICK;
                
                while (mainState == MENU) {
                    
                    menuselect = 0;
                    
                    switch (menurow) {
                        
                        case 1:
                            i1=3, i2=1, i3=1, i4=1, i5=1;
                            print_menu(pContext, &i1, &i2, &i3, &i4, &i5, &menurow, &ledstate);
            
                            if (menurow == 1 && menuselect == 1) {    
                                Display_doClearLines(hDisplay, 2, 9);
                                Display_print0(hDisplay, 6, 1, "Game  starting...");
                                Task_sleep(1000000 / Clock_tickPeriod);
                                Display_doClearLines(hDisplay, 2, 9);
                                mainState = GAME;
                                game(&i2cMPU, &i2cMPUParams);
                                menuState = PICK;
                                print_rows(pContext);
                            }
                            break;
                        
                        case 2:
                            i1=1, i2=3, i3=1, i4=1, i5=1;
                            print_menu(pContext, &i1, &i2, &i3, &i4, &i5, &menurow, &ledstate);
            
                            if (menurow == 2 && menuselect == 1) {  
                                menuState = OTHER;
                                help(pContext);
                                menuState = PICK;
                            }
                            break;

                        case 3:
                            i1=1, i2=1, i3=3, i4=1, i5=1;
                            print_menu(pContext, &i1, &i2, &i3, &i4, &i5, &menurow, &ledstate);
            
                            if (menurow == 3 && menuselect == 1) {
                                menuState = CALIB;
                                calib_start();
                                
                                while (calib_help == 2) {
                                    Task_sleep(500000 / Clock_tickPeriod);
                                }
                
                                left_rate=0, right_rate=0, up_rate=0, down_rate=0;
                                calibrate(&i2cMPU, &i2cMPUParams, &left_rate, &right_rate, &up_rate, &down_rate, calib_help);
                                update_rates(slippery, &left_rate, &right_rate, &up_rate, &down_rate);
                                calib_help = 2;
                                menuState = PICK;
                                print_rows(pContext);
                            }
                            break;
                            
                        case 4:
                            i1=1, i2=1, i3=1, i4=3, i5=1;
                            print_menu(pContext, &i1, &i2, &i3, &i4, &i5, &menurow, &ledstate);
            
                            if (menurow == 4 && menuselect == 1) {
                                PIN_setOutputValue( hLed, Board_LED0, !PIN_getOutputValue( Board_LED0 ) );
                                ledstate = PIN_getOutputValue(Board_LED0);
                                menurow = 4;
                                print_menu(pContext, &i1, &i2, &i3, &i4, &i5, &menurow, &ledstate);
                            }
                            break;
                        
                        case 5:
                            i1=1, i2=1, i3=1, i4=1, i5=3;
                            print_menu(pContext, &i1, &i2, &i3, &i4, &i5, &menurow, &ledstate);
            
                            if (menurow == 5 && menuselect == 1) {
                                menuState = OTHER;
                                Display_clear(hDisplay);
                                Display_print0(hDisplay, 6, 1, "Bye Bye!! :(");                        
                                Task_sleep(2000000 / Clock_tickPeriod);                        
                                Display_clear(hDisplay);
                                Display_close(hDisplay);
                                PIN_setOutputValue( hLed, Board_LED0, PIN_getOutputValue(0) );    
                                Task_sleep(100000 / Clock_tickPeriod);        
                    	        PIN_close(hPowerButton);
                                PINCC26XX_setWakeup(cPowerWake);
                    	        Power_shutdown(NULL,0);
                            }
                            break;
                            
                    }
                    
                    Task_sleep(10000 / Clock_tickPeriod);
            
                }
                break;
            
            case GAME:
                
                break;

        }
        
        // Do not remove sleep-call from here!
    	Task_sleep(1000000 / Clock_tickPeriod);
    	
    }
    // MPU9250 POWER OFF 
	// Because of loop forever, code never goes here
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);
}


void game(I2C_Handle *i2c_addr, I2C_Params *i2cParams_addr) {
    
    I2C_Handle      i2cMPU;
    I2C_Params      i2cMPUParams;
    i2cMPU = *i2c_addr;
    i2cMPUParams = *i2cParams_addr;
    
    pContext = DisplayExt_getGrlibContext(hDisplay);
    
    // TODO: alkuvalmistelut();
    lost = 0;
    level = 0;
    show_level(level);
    messagenum=7;
    field(pContext, dark, ledstate);
    memset(row1,0,4);
    memset(row2,0,4);
    memset(row3,0,4);
    memset(row4,0,4);
    memset(row5,0,4);
    
    gameState = READY;
    
    while (mainState == GAME) {
        switch (gameState) {
            
            case READ_SENSOR:
                read_mpu(&i2cMPU, &i2cMPUParams, &left_rate, &right_rate, &up_rate, &down_rate, &playerRight, &playerJump);
                update_player(pContext, &playerRight, &playerJump, &cold, &row5[0], &other_tree);
                check_if_lost(&lost, &row5[0], &playerRight, &playerJump);
                if (lost == 1) {
                    game_over(pContext, level);
                    mainState = MENU;
                }
                
                gameState = READY;
                break;
            
            case MSG_WAITING:
                // Reset message
                memset(payload,0,16);
                // Read message
                Receive6LoWPAN(&senderAddr, payload, 16);
                
                handle_msg(&senderAddr, &payload[0], &row1[0], &row2[0], &row3[0], &row4[0], &row5[0], dark, ledstate);
                update_road(pContext, &row1[0], &row2[0], &row3[0], &row4[0], &row5[0], other_tree, dark, ledstate);
                check_if_lost(&lost, &row5[0], &playerRight, &playerJump);
                if (lost == 1) {
                    game_over(pContext, level);
                    mainState = MENU;
                }
                if (senderAddr == IEEE80154_SERVER_ADDR) {
                    level++;
                    show_level(level);
                }
                gameState = READY;
                break;
                
            case MSG_PICK:
                messages();
                gameState = READY;
                break;
            
            case MSG_SEND:
                // send to everyone: 0xFFFF
                Send6LoWPAN(IEEE80154_SERVER_ADDR, msg, strlen(msg));
                StartReceive6LoWPAN();
                Display_doClearLines(hDisplay, 11, 11);
                Display_print0(hDisplay, 11, 0, "Message sent!");
                messagenum=8;
                gameState = READY;
                break;
                
        }
        // TODO: optimize
        Task_sleep(5000/Clock_tickPeriod);
    }
}


void messages(void) {
    switch(messagenum) {
        case 1:
            sprintf(msg, "Hello!");
            Display_print0(hDisplay, 11, 0, "Send \"Hello!\"");
            break;
        case 2:
            sprintf(msg, "GG!");
            Display_print0(hDisplay, 11, 0, "Send \"GG!\"");                  
            break;
        case 3:
            sprintf(msg, "Git Gud!");
            Display_print0(hDisplay,11, 0, "Send \"Git Gud!\"");              
            break;
        case 4:
            sprintf(msg, ";)");
            Display_print0(hDisplay, 11, 0, "Send \";)\"");
            break;
        case 5:
            sprintf(msg, "LOL!");
            Display_print0(hDisplay, 11, 0, "Send \"LOL!\"");
            break;
        case 6:
            sprintf(msg, "%dp!", level);
            Display_print0(hDisplay, 11, 0, "Send your score");
            break;
        case 7:
            Display_doClearLines(hDisplay, 11, 11);
            break;
    }
}


Int main(void) {

    /* Task variables */
	Task_Handle hMainTask;
	Task_Params mainTaskParams;
	Task_Handle hCommTask;
	Task_Params commTaskParams;
	Clock_Handle clkHandle;
    Clock_Params clkParams;

    /* Initialize board */
    Board_initGeneral();
    Board_initI2C();

	/* Power Button */
	hPowerButton = PIN_open(&sPowerButton, cPowerButton);
	if(!hPowerButton) {
		System_abort("Error initializing power button shut pins\n");
	}
	if (PIN_registerIntCb(hPowerButton, &powerButtonFxn) != 0) {
		System_abort("Error registering power button callback function");
	}

    /* Button0 */
    hButton0 = PIN_open(&sButton0, cButton0);
    if(!hButton0) {
        System_abort("Error initializing button pins\n");
    }
    if (PIN_registerIntCb(hButton0, &buttonFxn) != 0) {
        System_abort("Error registering button callback function");
    }

    /* Leds */
    hLed = PIN_open(&sLed, cLed);
    if(!hLed) {
        System_abort("Error initializing LED pin\n");
    }
    
    /* Init clock */
    Clock_Params_init(&clkParams);
    clkParams.period = 250000 / Clock_tickPeriod;
    clkParams.startFlag = TRUE;
    
    /* Create clock */
    clkHandle = Clock_create((Clock_FuncPtr)clkFxn, 1000000 / Clock_tickPeriod, &clkParams, NULL);
    if (clkHandle == NULL) {
       System_abort("Clock create failed");
    }

    /* Init Main Task */
    Task_Params_init(&mainTaskParams);
    mainTaskParams.stackSize = STACKSIZE;
    mainTaskParams.stack = &mainTaskStack;
    mainTaskParams.priority=1;

    hMainTask = Task_create(mainTask, &mainTaskParams, NULL);
    if (hMainTask == NULL) {
    	System_abort("Task create failed!");
    }

    /* Init Communication Task */
    Task_Params_init(&commTaskParams);
    commTaskParams.stackSize = STACKSIZE;
    commTaskParams.stack = &commTaskStack;
    commTaskParams.priority=1;

    Init6LoWPAN();
    
    hCommTask = Task_create(commTask, &commTaskParams, NULL);
    if (hCommTask == NULL) {
    	System_abort("Task create failed!");
    }

    /* Send OK to console */
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}

