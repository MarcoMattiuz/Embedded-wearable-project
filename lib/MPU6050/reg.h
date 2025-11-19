#ifndef __REG_H__
#define __REG_H__

#include <stdio.h>
#include <stdbool.h>

//I2C config 
#define I2C_MASTER_NUM       I2C_NUM_1 
#define I2C_SDA_PIN          GPIO_NUM_21
#define I2C_SCL_PIN          GPIO_NUM_22
#define I2C_FREQ_HZ          100000

#define MPU6050_ADDR         0x68 //I2C address of MPU6050
#define WHO_AM_I             0x75

//MPU6050 registers
//sensor internal temperature
#define MPU6050_TEMP_OUT_H   0x41
#define MPU6050_TEMP_OUT_L   0x42

//gyro registers
#define MPU6050_GYRO_XOUT_H  0x43
#define MPU6050_GYRO_XOUT_L  0x44
#define MPU6050_GYRO_YOUT_H  0x45
#define MPU6050_GYRO_YOUT_L  0x46
#define MPU6050_GYRO_ZOUT_H  0x47
#define MPU6050_GYRO_ZOUT_L  0x48

//acc registers
#define ACC_CONFIG           0x1C
/*
    7:       XA_SELF_TEST
    6:       YA_SELF_TEST
    5:       ZA_SELF_TEST
    4, 3:    AFS_SELF
    2, 1, 0: -

    AFS_SELF | g_ramge | bin
        0         2      0x00
        1         4      0x08
        2         8      0x10
        3        16      0x18
*/
#define MPU6050_ACC_G_RANGE  0x00
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_ACCEL_XOUT_L 0x3C
#define MPU6050_ACCEL_YOUT_H 0x3D
#define MPU6050_ACCEL_YOUT_L 0x3E
#define MPU6050_ACCEL_ZOUT_H 0x3F
#define MPU6050_ACCEL_ZOUT_L 0x50

#define PWR_MGMT_1           0x6B
#define SMPLRT_DIV           0x19
#define CONFIG               0x1A
#define GYRO_CONFIG          0x1B

#define READING_BYTE 6
#define WRITING_BYTE 6

typedef struct {
    int16_t a_x; // axis X
    int16_t a_y; // axis Y
    int16_t a_z; // axis Z
} Three_Axis_t;

typedef uint8_t Reading_Buffer_t[READING_BYTE];
typedef uint8_t Writing_Buffer_t[WRITING_BYTE];

extern int step_cntr;
#define STEP_COUNTER_INC(x) ((x)++)
#define SMOOTHING_FACTOR 4
/*
    We need two thresholds for M
      high 
      low  
*/
#define M_REST         16384 //resting acceleration
#define THRESHOLD_HIGH 4500
#define THRESHOLD_LOW  4100

#endif