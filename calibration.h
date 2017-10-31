#include <ti/drivers/I2C.h>

void minmaxmean(float *array, uint8_t size, float *min, float *max, float *mean);

void instructions1();
void rest_part(I2C_Handle *i2c_addr, I2C_Params *i2cParams_addr, float *rest_gz_min_addr, float *rest_gz_max_addr, float *rest_gx_min_addr, float *rest_gx_max_addr);

void instructions2();
void instructions3();
void instructions4();
void instructions5();
void move_part(I2C_Handle *i2c_addr, I2C_Params *i2cParams_addr, float *move_rate, uint8_t direction);

void oops(uint8_t direction);

void calibrate(I2C_Handle *i2c_addr, I2C_Params *i2cParams_addr, float *left_rate, float *right_rate, float *up_rate, float *down_rate, uint8_t help);

