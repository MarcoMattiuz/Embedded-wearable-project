/*
    ! REGISTERS and STRUCTS and MACROS used by MPU6050
*/
#ifndef __REG_H__
#define __REG_H__

#include <stdio.h>
#include <stdbool.h>
#include "../SHARED/global_param.h"

/*
    bit 7 → 0x80   (128)
    bit 6 → 0x40   (64)
    bit 5 → 0x20   (32)
    bit 4 → 0x10   (16)
    bit 3 → 0x08   (8)
    bit 2 → 0x04   (4)
    bit 1 → 0x02   (2)
    bit 0 → 0x01   (1)
*/  

#define DELAY_10 pdMS_TO_TICKS(10)
#define DELAY_20 pdMS_TO_TICKS(20)
#define DELAY_30 pdMS_TO_TICKS(30)
#define DELAY_40 pdMS_TO_TICKS(40)
#define DELAY_50 pdMS_TO_TICKS(50)

#define ERR ESP_ERR_INVALID_ARG

#define PWR_MGMT_1        0x6B
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
#define PWR_MGMT_1_CONFIG 0x08
#define PWR_MGMT_1_CLK    0x01
#define SMPLRT_DIV        0x19
#define CONFIG            0x1A
#define GYRO_CONFIG       0x1B

//used for debug
#define MPU6050_WHO_AM_I 0x75

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
#define USER_CTRL_BIT_FIFO_EN  0x40   // ! bit 6:   01000000
#define USER_CTRL_BIT_FIFO_RST 0x04   // ! bit 2:   00000100
#define FIFO_EN_BIT_ACCEL      0x08   // ! bit 3:   00001000
#define FIFO_EN_BIT_XG         0x40   // ! bit 6:   01000000
#define FIFO_EN_BIT_YG         0X20   // ! bit 5:   00100000
#define FIFO_EN_BIT_ZG         0x10   // ! bit 4:   00010000
#define FIFO_DISABLE           0x00   // ! reg set: 00000000
#define FIFO_EMPTY             ESP_ERR_NOT_FOUND
#define RESET_FIFO             ESP_ERR_INVALID_SIZE
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

//INTERRUPT
#define MPU6050_INT_STATUS                    0x3A
#define MPU6050_INT_STATUS_BIT_FIFO_OFLOW_INT 0x10 // bit 4
#define MPU6050_INT_ENABLE                    0x38
#define MPU6050_INT_ENABLE_BIT_FIFO_OFLOW_INT 0x10 // bit 4
#define FIFO_OVERFLOW(x) (x & MPU6050_INT_STATUS_BIT_FIFO_OFLOW_INT)

//sensor internal temperature registers
#define MPU6050_TEMP_OUT_H 0x41
#define MPU6050_TEMP_OUT_L 0x42

//gyro registers 
#define MPU6050_GYRO_CONFIG 0x1B
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
#define MPU6050_GYRO_RANGE  0x18
#define SENS_GYRO_RANGE     16.4
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

    AFS_SELF | g_range | hex
        0         2      0x00
        1         4      0x08
        2         8      0x10
        3        16      0x18
*/
#define MPU6050_ACC_G_RANGE  0x08
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_ACCEL_XOUT_L 0x3C
#define MPU6050_ACCEL_YOUT_H 0x3D
#define MPU6050_ACCEL_YOUT_L 0x3E
#define MPU6050_ACCEL_ZOUT_H 0x3F
#define MPU6050_ACCEL_ZOUT_L 0x40

#define READING_BYTE 6
#define WRITING_BYTE 6

//step counter/gyro structs&macros
typedef struct {
    int16_t a_x;
    int16_t a_y;
    int16_t a_z;
} Three_Axis_t;

typedef struct {
    float a_x; 
    float a_y; 
    float a_z; 
} Three_Axis_final_t;

typedef struct {
    int16_t g_x;
    int16_t g_y;
    int16_t g_z;
} Gyro_Axis_t;

typedef struct {
    float g_x;
    float g_y;
    float g_z;
} Gyro_Axis_final_t;

typedef struct {
    float angle;    // angle of accumulation (°)
    bool  rotating; // current state: rotating or stand
} WristState_t;

typedef struct {
    float integrated_angle;
    uint32_t last_trigger;
} Rotation_t;

typedef struct {
    float x;
    float y;
    float z;
} Vec3_t;

typedef struct {
    float pitch;  
    float yaw;    
} Orientation_t;

// #define SMOOTHING_FACTOR 4
/*
    We need two thresholds for M
      high 
      low  
*/
#define GRAVITY             9.8f     // m/s²
#define ACCEL_SCALE         16384.0f //resting acceleration
#define THRESHOLD_H         1.2f
#define THRESHOLD_L         0.8f
#define DEG_TO_RAD          0.01745329252f  // π/180
#define WRIST_ROT_THRESHOLD 60.0f //min wrist rotation
#define MIN_ROT_ANGLE       20.0f //min accumulate ang for confirm rotation
#define DT                  0.01f //time between samples relate to 100Hz
#define REFRACT_MS          500   //ro avoid double triggers

#endif