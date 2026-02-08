#include "../include/roll_pitch.h"

static Rotation_t    rotation = {0.0f, 0};
static Orientation_t orient   = {0, 0, 0};

static float M_lp = GRAVITY;
static bool up = false;
static uint32_t last_step = 0;
/*
   MOTION ANALYSIS
*/
bool verify_step(ACC_Three_Axis_t *ax)
{
    /*
        COMPUTE MAGNITUDE

        M = sqrt(x² + y² + z²)

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
    float M = sqrtf(ax->a_x * ax->a_x +
                    ax->a_y * ax->a_y +
                    ax->a_z * ax->a_z);

    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // low-pass filer esponetional average
    M_lp = 0.95f * M_lp + 0.05f * M;
    float M_hp = M - M_lp;

    // avoid double triggers, 500ms is an average step time 
    // for a normal walking pace (120 steps/min)
    if ((now - last_step) < 500)
        return false;

    if (!up && M_hp > THRESHOLD_H)
    {
        // rising edge
        // this means that one step is detected when M 
        // raises above THRESHOLD_H and up is false (down)
        up = true;
        last_step = now;
        return true;
    }
    else if (up && M_hp < THRESHOLD_L)
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
    if ((now - rotation.last_trigger) < REFRACT_MS) 
    {
        return false;
    }

    /*
        angular velocity °/s
        set wrist rotation axis, in this case Z axis, 
        but it can be changed depending on the orientation of the device
    */
   float w = fabsf(g->g_x);   
    
    // °/s * s -> °
    rotation.integrated_angle += w * DT;   

    // Detect wrist rotation 
    if (rotation.integrated_angle >= WRIST_ROT_THRESHOLD) 
    {
        // reset rotation state
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

    if (wrist) 
    {
        ESP_LOGI("VERIFY_MOTION", "WRIST ROTATION DETECTED -----------------------------------------------------------------");
    }

    if (step) 
    {
        global_parameters.step_cntr ++;
        ESP_LOGI("VERIFY_MOTION", "STEPS: %d -----------------------------------------------------------------------------", 
            global_parameters.step_cntr);
    }
}


void update_orientation(const GYRO_Three_Axis_t *gyro, const ACC_Three_Axis_t  *acc)
{
    // ACC angles 
    // lateral inclination angle
    float acc_roll  = atan2f(acc->a_y, acc->a_z) * RAD_TO_DEG;
    // longitudinal inclination angle
    float acc_pitch = atan2f(-acc->a_x,
                       sqrtf(acc->a_y * acc->a_y +
                             acc->a_z * acc->a_z)) * RAD_TO_DEG;

    // angle = ∫ ω dt

    // GYRO integration 
    orient.roll  += gyro->g_x * DT;
    orient.pitch += gyro->g_y * DT;

    // drift becasue we need a magnetometer to correct it
    // (we don't have it), 
    orient.yaw += gyro->g_z * DT;

    // Complementary filter 
    orient.roll  = ALPHA * orient.roll  + (1.0f - ALPHA) * acc_roll;
    orient.pitch = ALPHA * orient.pitch + (1.0f - ALPHA) * acc_pitch;
}

void get_orientation_vector(GYRO_Three_Axis_t *gyro_data, GYRO_Three_Axis_t *tmp)
{
    /*
        This function create a versor of gyro data in which:
        X = cos(pitch) * cos(yaw)
        Y = cos(pitch) * sin(yaw)
        Z = sin(pitch)

        Needs to convert the angles from degrees to radians 
        before applying the trigonometric functions and get
        a vector that represent the orientation of the device in 3D space.
    */

    // ° -> rad
    float pitch = orient.pitch * DEG_TO_RAD;
    float yaw   = orient.yaw   * DEG_TO_RAD;
    
    tmp->g_x = cosf(pitch) * cosf(yaw);
    tmp->g_y = cosf(pitch) * sinf(yaw);
    tmp->g_z = sinf(pitch);
}
