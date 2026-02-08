#ifndef __ROLL_PITCH_H__
#define __ROLL_PITCH_H__

// #include "driver/i2c.h"
// #include "esp_err.h"
// #include "freertos/queue.h"
#include "mpu6050.h"

// Complementary filter coefficients
/*
    2g = 19.62 m/s²
    my peaks have to stay under this value
*/
#define THRESHOLD_H         4.0f     
#define THRESHOLD_L         0.5f    
#define ALPHA               0.9f     // the weight for the gyroscope data
#define REFRACT_MS          500      // ro avoid double triggers
#define MIN_ROT_ANGLE       10.0f    // min accumulate ang for confirm rotation
#define WRIST_ROT_THRESHOLD 60.0f    // min wrist rotation
#define DT                  0.008f   // time between samples relate to 125Hz

// MACROS
#define DEG_TO_RAD 0.01745329252f  // π/180
#define RAD_TO_DEG 57.2957795131f  // 180/π
#define DT         0.008f          // 8 ms -> sample period

// Types
typedef struct {
    float    integrated_angle;
    uint32_t last_trigger;
} Rotation_t;

typedef struct {
    float roll;
    float pitch;  
    float yaw;
} Orientation_t;

bool verify_step           (ACC_Three_Axis_t *ax);
bool verify_wrist_rotation (GYRO_Three_Axis_t *g);
void verify_motion         (ACC_Three_Axis_t *acc_data, GYRO_Three_Axis_t *gyro_data);

void update_orientation     (const GYRO_Three_Axis_t *gyro, const ACC_Three_Axis_t  *acc);
void get_orientation_vector (GYRO_Three_Axis_t *gyro_data, GYRO_Three_Axis_t *tmp);

#endif

