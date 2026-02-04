#include "../include/roll_pitch.h"
#include <math.h>

static float roll  = 0.0f;
static float pitch = 0.0f;

Rotation_t rotation;

void roll_pitch_init(void) 
{
    roll = 0.0f;
    pitch = 0.0f;
    rotation.integrated_angle = 0.0f;
    rotation.last_trigger = 0;
}

void roll_pitch_update(ACC_Three_Axis_t acc_data, GYRO_Three_Axis_t gyro_data)
{

    // Calculate roll and pitch from accelerometer data
    float accel_roll  = atan2f(acc_data.a_y, acc_data.a_z) * 180.0f / M_PI;
    float accel_pitch = atan2f(-acc_data.a_x, sqrtf(acc_data.a_y * acc_data.a_y + acc_data.a_z * acc_data.a_z)) * 180.0f / M_PI;

    // Gyroscope readings in degrees per second
    float roll_rate  = gyro_data.g_x / GYRO_SCALE;
    float pitch_rate = gyro_data.g_y / GYRO_SCALE;

    // Update roll and pitch using gyroscope data
    roll += roll_rate * DT;
    pitch += pitch_rate * DT;

    // Complementary filter to combine accelerometer and gyroscope data
    roll = ALPHA * roll + (1.0f - ALPHA) * accel_roll;
    pitch = ALPHA * pitch + (1.0f - ALPHA) * accel_pitch;
}

float roll_get(void) 
{
    return roll;
}

float pitch_get(void) 
{
    return pitch;
}

/*
   MOTION ANALYSIS
*/
bool verify_step(const ACC_Three_Axis_t *ax)
{
    
    float M = sqrt((ax->a_x * ax->a_x) +
                   (ax->a_y * ax->a_y) +
                   (ax->a_z * ax->a_z));

    static bool up = false;

    if (!up && M > (ACCEL_SCALE + THRESHOLD_H))
    { 
        // rising edge
        // this means that one step is detected when M raises above TH_H and up is false (down)
        up = true;
        return true;
    }
    else if (up && M < (ACCEL_SCALE + THRESHOLD_L))
    { 
        // falling edge
        up = false;
    }

    return false;
}

bool verify_wrist_rotation(const GYRO_Three_Axis_t *g)
{
    // refractory window, avoid double triggers
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if ((now - rotation.last_trigger) < REFRACT_MS) {
        return false;
    }

    // angular velocity °/s
    float w = fabsf(g->g_z);   // set wrist rotation axis

    // noise threshold
    if (w < MIN_ROT_ANGLE) {
        // leaky integrator to forget noise
        rotation.integrated_angle *= 0.95f;
        return false;
    }
    
    // °/s * s -> °
    rotation.integrated_angle += w * DT;   

    if (rotation.integrated_angle >= WRIST_ROT_THRESHOLD) {
        rotation.integrated_angle = 0.0f;
        rotation.last_trigger = now;
        return true;
    }

    return false;
}

void verify_motion(const ACC_Three_Axis_t *acc_data, const GYRO_Three_Axis_t *gyro_data) 
{
    bool step  = verify_step(acc_data);
    bool wrist = verify_wrist_rotation(gyro_data);

    if (step && !wrist)
    {
        global_parameters.step_cntr ++;
        printf("STEPS: %d\n", global_parameters.step_cntr);
    }
    else if (wrist)
    {
        printf("WRIST ROTATION DETECT\n");

        // EventType evt = EVT_GYRO;
        // xQueueSend(event_queue, &evt, 0);
    }
}