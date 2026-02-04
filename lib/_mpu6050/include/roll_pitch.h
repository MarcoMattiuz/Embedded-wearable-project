#ifndef __ROLL_PITCH_H__
#define __ROLL_PITCH_H__

#include "driver/i2c.h"
#include "esp_err.h"
#include "mpu6050.h"

// Complementary filter coefficients
#define ALPHA               0.7f // The weight for the gyroscope data
#define THRESHOLD_H         1.2f
#define THRESHOLD_L         0.8f
#define REFRACT_MS          500      // ro avoid double triggers
#define MIN_ROT_ANGLE       20.0f    //min accumulate ang for confirm rotation
#define WRIST_ROT_THRESHOLD 60.0f    //min wrist rotation
#define DT                  0.01f    //time between samples relate to 100Hz

typedef struct {
    float    integrated_angle;
    uint32_t last_trigger;
} Rotation_t;

// Function prototypes
void  roll_pitch_init   (void);
void  roll_pitch_update (ACC_Three_Axis_t acc_data, GYRO_Three_Axis_t gyro_data);
float roll_get          (void);
float pitch_get         (void);

bool verify_step           (const ACC_Three_Axis_t *ax);
bool verify_wrist_rotation (const GYRO_Three_Axis_t *g);
void verify_motion         (const ACC_Three_Axis_t *acc_data, const GYRO_Three_Axis_t *gyro_data);

#endif

