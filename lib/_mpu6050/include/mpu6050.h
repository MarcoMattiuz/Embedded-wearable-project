#ifndef __MPU6050_H__
#define __MPU6050_H__

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "../SHARED/global_param.h"
#include <stdint.h>
#include <math.h>

/******************
*   bit 0 = 0x01  *
*   bit 1 = 0x02  *
*   bit 2 = 0x04  *
*   bit 3 = 0x08  *
*   bit 4 = 0x10  *
*   bit 5 = 0x20  *
*   bit 6 = 0x40  *
*   bit 7 = 0x80  *
*******************/

// MPU6050 I2C address
#define MPU6050_ADDR 0x68

// MPU6050 Register Addresses
#define MPU6050_REG_PWR_MGMT_1 0x6B
/*

    7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 
    -----------------------------
    0   0   0   0   1   0   0   0
    ^   ^   ^       ^   ^   ^   ^
    |   |   |       |   |___|___| CLKSEL
    |   |   |       | TEMP_DIS
    |   |   | CYCLE
    |   | SLEEP
    | DEVICE_RESET
*/
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H  0x43
#define ACCEL_CONFIG             0x1C
/*
    7:       XA_SELF_TEST
    6:       YA_SELF_TEST
    5:       ZA_SELF_TEST
    4, 3:    AFS_SELF
    2, 1, 0: -

    AFS_SELF | g_range | hex
        0         2      0x00
        1         4      0x08
        2         8      0x10
        3        16      0x18
*/
#define ACCEL_G_RANGE 0x08
#define GYRO_CONFIG   0x1B
/*
    7:       XG_ST
    6:       YG_ST
    5:       ZG_ST
    4, 3:    FS_SELF
    2, 1, 0: -
                (°/s)
    FS_SELF | full_range |  hex   | sens
       0         250       0x00     131.0
       1         500       0x08     65.5
       2         1000      0x10     32.8
       3         2000      0x18     16.4
*/
#define GYRO_RANGE 0x18

#define SMPLRT_DIV 0x19
#define CONFIG     0x1A
#define USER_CTRL  0x6A 
/*
    set USR_CTRL register to:

    7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
    -----------------------------
    0 | 1 | 0 | 0 | 0 | 1 | 0 | 0
        ^               ^
        | FIFO_EN       | FIFO_RESET
*/
#define INT_ENABLE 0x38
/*
    set INT_ENABLE register to:

    7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
    -----------------------------
    0 | 0 | 0 | 1 | 0 | 0 | 0 | 0
                ^
                |
                | FIFO_OFLOW_EN
*/
#define FIFO_EN 0x23
/*
    set FIFO_EN register to:

    7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
    -----------------------------
    1 | 1 | 1 | 1 | 1 | 0 | 0 | 0
    ^   ^    ^   ^   ^
    |   |    |   |   | ACCEL_FIFO_EN
    |   |    |   | ZG_FIFO_EN
    |   |    | YG_FIFO_EN
    |   | XG_FIFO_EN
    | TEMP_FIFO_EN
*/
#define MPU6050_FIFO_DATA_R_W 0x74
// DIM SAMPLE IN BYTES
#define MPU6050_FIFO_COUNT_H  0x72 
#define MPU6050_FIFO_COUNT_L  0x73
#define MPU6050_INT_STATUS    0x3A
/*
    set INT_STATUS register to:

    7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
    -----------------------------
    1 | 0 | 0 | 1 | 0 | 0 | 0 | 0
                ^
                |
                | FIFO_OFLOW_INT
*/

//MACROS
#define FIFO_SAMPLE_SIZE 14
#define ACCEL_SCALE      16384.0f // for ±2g range
#define GYRO_SCALE       131.0f   // for ±250°/s range
#define GRAVITY          9.8f     // m/s²

//TYPES
typedef struct {
    int16_t a_x;
    int16_t a_y;
    int16_t a_z;
} Raw_Data_acc;

typedef struct {
    int16_t g_x;
    int16_t g_y;
    int16_t g_z;
} Raw_Data_gyro;

typedef struct {
    float a_x;
    float a_y;
    float a_z;
} ACC_Three_Axis_t; // m/s²

typedef struct {
    float g_x;
    float g_y;
    float g_z;
} GYRO_Three_Axis_t; // °/s

typedef struct {
    float a_x_sum;
    float a_y_sum;
    float a_z_sum;
} ACC_accumulated;

typedef struct {
    float g_x_sum;
    float g_y_sum;
    float g_z_sum;
} GYRO_accumulated;
    
//FUNCTIONS
void      mpu6050_set_handle     (i2c_master_dev_handle_t dev_handle);
esp_err_t mpu6050_init           ();
bool      mpu6050_check_overflow (void);

int get_fifo_size (void);

esp_err_t mpu6050_read_raw_data (Raw_Data_acc *raw_acc, Raw_Data_gyro *raw_gyro);

void mpu6050_convert_accel (Raw_Data_acc *raw_acc, ACC_Three_Axis_t *acc_data);
void mpu6050_convert_gyro  (Raw_Data_gyro *raw_gyro, GYRO_Three_Axis_t *gyro_data);

void mpu6050_calibrate (float *accel_bias, float *gyro_bias);

#endif