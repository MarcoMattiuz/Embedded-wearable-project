#include "../include/roll_pitch.h"

static float         roll     = 0.0f;
static float         pitch    = 0.0f;
Rotation_t           rotation = {0.0f, 0};
static Orientation_t orient   = {0, 0, 0};

float accMagFiltered = 0;
float accMagPrev     = 0;

unsigned long lastStepTime = 0;

// void roll_pitch_init(void) 
// {
//     roll = 0.0f;
//     pitch = 0.0f;
//     rotation.integrated_angle = 0.0f;
//     rotation.last_trigger = 0;
// }

// void roll_pitch_update(ACC_Three_Axis_t acc_data, GYRO_Three_Axis_t gyro_data)
// {

//     // Calculate roll and pitch from accelerometer data
//     float accel_roll  = atan2f(acc_data.a_y, acc_data.a_z) * 180.0f / M_PI;
//     float accel_pitch = atan2f(-acc_data.a_x, sqrtf(acc_data.a_y * acc_data.a_y + acc_data.a_z * acc_data.a_z)) * 180.0f / M_PI;

//     // Gyroscope readings in degrees per second
//     float roll_rate  = gyro_data.g_x / GYRO_SCALE;
//     float pitch_rate = gyro_data.g_y / GYRO_SCALE;

//     // Update roll and pitch using gyroscope data
//     roll += roll_rate * DT;
//     pitch += pitch_rate * DT;

//     // Complementary filter to combine accelerometer and gyroscope data
//     roll = ALPHA * roll + (1.0f - ALPHA) * accel_roll;
//     pitch = ALPHA * pitch + (1.0f - ALPHA) * accel_pitch;
// }

// float roll_get(void) 
// {
//     return roll;
// }

// float pitch_get(void) 
// {
//     return pitch;
// }

/*
   MOTION ANALYSIS
*/
bool verify_step(ACC_Three_Axis_t *ax)
{
    /*
        Compute Magnitude
        It rappresents the lenght of a 3D vector,
        in this case the acceleration vector composed 
        by the 3 axis of the accelerometer.

        M oscillates around GRAVITY (9.8 m/s²) when 
        the device is still, but when a step is taken, 
        M will show a peak that can be detected by 
        setting a threshold.

        Being in 1D is easier to filtrate noise and detect steps, 
        but this method is less accurate and can be affected by other movements.
    */
    float M = sqrt((ax->a_x * ax->a_x) +
                   (ax->a_y * ax->a_y) +
                   (ax->a_z * ax->a_z));

    ESP_LOGI("VERIFY_STEP", "M: %.2f", M);

    static bool up = false;

    if (!up && M > (GRAVITY + THRESHOLD_H))
    { 
        // rising edge
        // this means that one step is detected when M 
        // raises above THRESHOLD_H and up is false (down)
        up = true;
        return true;
    }
    else if (up && M < (GRAVITY + THRESHOLD_L))
    { 
        // falling edge
        up = false;
    }

    return false;
}

bool verify_wrist_rotation(GYRO_Three_Axis_t *g)
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

void verify_motion(ACC_Three_Axis_t *acc_data, GYRO_Three_Axis_t *gyro_data) 
{
    bool step  = verify_step(acc_data);
    bool wrist = verify_wrist_rotation(gyro_data);

    if (step && !wrist)
    {
        global_parameters.step_cntr ++;
        ESP_LOGI("VERIFY_MOTION", "STEPS: %d -------------------------------------------------", 
            global_parameters.step_cntr);
    }
    else if (wrist)
    {
        ESP_LOGI("VERIFY_MOTION", "WRIST ROTATION DETECTED ---------------------------------------------");

        // EventType evt = EVT_GYRO;
        // xQueueSend(event_queue, &evt, 0);
    }
}

void update_orientation(const GYRO_Three_Axis_t *gyro, const ACC_Three_Axis_t  *acc)
{
    // ACC angles 
    float acc_roll  = atan2f(acc->a_y, acc->a_z) * RAD_TO_DEG;
    float acc_pitch = atan2f(-acc->a_x,
                       sqrtf(acc->a_y * acc->a_y +
                             acc->a_z * acc->a_z)) * RAD_TO_DEG;

    // GYRO integration 
    orient.roll  += gyro->g_x * DT;
    orient.pitch += gyro->g_y * DT;
    orient.yaw   += gyro->g_z * DT;   // drift inevitabile

    // Complementary filter 
    orient.roll  = ALPHA * orient.roll  + (1.0f - ALPHA) * acc_roll;
    orient.pitch = ALPHA * orient.pitch + (1.0f - ALPHA) * acc_pitch;
}

void get_orientation_vector(GYRO_Three_Axis_t *gyro_data, GYRO_Three_Axis_t *tmp)
{
    // this function create a verson of gyro data in which:
    // X = cos(pitch) * cos(yaw)
    // Y = cos(pitch) * sin(yaw)
    // Z = sin(pitch)

    // orient is degrees
    orient.pitch += gyro_data->g_y * DT;
    orient.yaw   += gyro_data->g_z * DT;

    // ° -> rad
    float pitch = orient.pitch * DEG_TO_RAD;
    float yaw   = orient.yaw   * DEG_TO_RAD;
    
    tmp->g_x = cosf(pitch) * cosf(yaw);
    tmp->g_y = cosf(pitch) * sinf(yaw);
    tmp->g_z = sinf(pitch);
}
