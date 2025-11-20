/*
    ! REGISTERS and STRUCTS and MACROS used by MPU6050
*/

#ifndef __REG_H__
#define __REG_H__

#include <stdio.h>
#include <stdbool.h>

//FIFO registers
#define MPU6050_USER_CTRL 0x6A
/*
    7: -
    6: FIFO_EN
    5: I2C_MST_EN
    4: I2C_IF_DIS
    3: -
    2: FIFO_RESET
    1: I2C_MST_RESET
    0: SIG_COND_RESET
*/
#define MPU6050_FIFO_EN 0x23
//configuration values  
#define USER_CTRL_BIT_FIFO_EN  0x40   // bit 6
#define USER_CTRL_BIT_FIFO_RST 0x04   // bit 2
#define FIFO_EN_BIT_ACCEL      0x08   // bit 3
/*
    Determines which sensor data are loaded in the FIFO buffer:
    7: TEMP_FIFO_EN
    6: XG_FIFO_EN
    5: YG_FIFO_EN
    4: ZG_FIFO_EN
    3: ACCEL_FIFO_EN
    2: SLV2_FIFO_EN
    1: SLV1_FIFO_EN
    0: SLV0_FIFO_EN

    eg:
    I want write accelerometer data in the FIFO -> set MPU6050_FIFO_EN.MPU6050_FIFO_EN_BIT_ACCEL to 1 
*/
#define MPU6050_FIFO_COUNT_H  0x72 
#define MPU6050_FIFO_COUNT_L  0x73 // 0x72 U 0x73 -> dim FIFO
#define MPU6050_FIFO_DATA_R_W 0x74 // where wr/rd data 

//sensor internal temperature registers
#define MPU6050_TEMP_OUT_H 0x41
#define MPU6050_TEMP_OUT_L 0x42

//gyro registers    
#define MPU6050_GYRO_XOUT_H 0x43
#define MPU6050_GYRO_XOUT_L 0x44
#define MPU6050_GYRO_YOUT_H 0x45
#define MPU6050_GYRO_YOUT_L 0x46
#define MPU6050_GYRO_ZOUT_H 0x47
#define MPU6050_GYRO_ZOUT_L 0x48

//acc registers
#define MPU6050_ACCEL_CONFIG 0x1C
/*
    7:       XA_SELF_TEST
    6:       YA_SELF_TEST
    5:       ZA_SELF_TEST
    4, 3:    AFS_SELF
    2, 1, 0: -

    AFS_SELF | g_ramge | hex
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
#define MPU6050_ACCEL_ZOUT_L 0x40

#define PWR_MGMT_1  0x6B
#define SMPLRT_DIV  0x19
#define CONFIG      0x1A
#define GYRO_CONFIG 0x1B

#define READING_BYTE 6
#define WRITING_BYTE 6

//step counter structs/macros
typedef struct {
    int16_t a_x; // axis X
    int16_t a_y; // axis Y
    int16_t a_z; // axis Z
} Three_Axis_t;

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