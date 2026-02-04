#ifndef __MPU6050_H__
#define __MPU6050_H__

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "../SHARED/global_param.h"
#include <stdint.h>

#define MPU6050_ADDR 0x68

// MPU6050 Register Addresses
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H  0x43
#define ACCEL_CONFIG 0x1C
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
#define ACCEL_G_RANGE 0x00
#define GYRO_CONFIG  0x1B
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
#define GYRO_RANGE 0x00

//MACROS
#define ACCEL_SCALE 16384.0f // for ±2g range
#define GYRO_SCALE  131.0f   // for ±250°/s range
#define GRAVITY     9.8f     // m/s²

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

typedef struct
{
    float roll;
    float pitch;
} GyroData_t;

//FUNCTIONS
void      mpu6050_set_handle (i2c_master_dev_handle_t dev_handle);
esp_err_t mpu6050_init       ();

esp_err_t mpu6050_read_raw_data (Raw_Data_acc *raw_acc, Raw_Data_gyro *raw_gyro);
void      mpu6050_convert_accel (Raw_Data_acc *raw_acc, ACC_Three_Axis_t *acc_data);
void      mpu6050_convert_gyro  (Raw_Data_gyro *raw_gyro, GYRO_Three_Axis_t *gyro_data);

void mpu6050_calibrate (float *accel_bias, float *gyro_bias);

// Roll and pitch calculation functions
void  roll_pitch_init   (void);
void  roll_pitch_update (float accel_x, float accel_y, float accel_z, float gyro_x, float gyro_y, float gyro_z);
float roll_get          (void);
float pitch_get         (void);

#endif