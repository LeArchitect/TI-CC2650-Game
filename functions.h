#include <ti/drivers/I2C.h>

// Before main loop
void intro(tContext *pContext);
void check_sensors(I2C_Handle *i2c_addr, I2C_Params *i2cParams_addr, uint8_t *slippery, uint8_t *other_tree, uint8_t *dark, uint8_t *cold);
void update_rates(uint8_t slippery, float *left_rate, float *right_rate, float *up_rate, float *down_rate);

// Menu functions
void print_lines(tContext *pContext);
void print_menu(tContext *pContext, uint8_t *i1, uint8_t *i2, uint8_t *i3, uint8_t *i4, uint8_t *i5, uint8_t *menurow, uint8_t *ledstate);
void help(tContext *pContext);
void calib_start(void);

// Game functions
void field(tContext *pContext, uint8_t dark, uint8_t ledstate);
void handle_msg(uint16_t *senderAddr, char *payload, uint8_t *row1, uint8_t *row2, uint8_t *row3, uint8_t *row4, uint8_t *row5, uint8_t dark, uint8_t ledstate);
void update_road_row(tContext *pContext, uint8_t *row, uint8_t row_number, uint8_t other_tree);
void update_road(tContext *pContext, uint8_t *row1, uint8_t *row2, uint8_t *row3, uint8_t *row4, uint8_t *row5, uint8_t other_tree, uint8_t dark, uint8_t ledstate);
void read_mpu(I2C_Handle *i2c_addr, I2C_Params *i2cParams_addr, float *left_rate, float *right_rate, float *up_rate, float *down_rate, uint8_t *playerRight, uint8_t *playerJump, uint8_t *change);
void update_player(tContext *pContext, uint8_t *playerRight, uint8_t *playerJump, uint8_t *cold, uint8_t *row5, uint8_t *other_tree, uint8_t change);
void check_if_lost(uint8_t *lost, uint8_t *row5, uint8_t *playerRight, uint8_t *playerJump);
void show_level(uint16_t level);
void game_over(tContext *pContext, uint16_t level);

