#include "MPU6050_api.h"

// static bool fifo_initialized = false;
Rotation_t rotation = {0.0f, 0};
static Orientation_t orient = {0};

esp_err_t mpu6050_write_reg(struct i2c_device *device, uint8_t reg_to_write, uint8_t val_to_write)
{

    uint8_t buf[2] = {reg_to_write, val_to_write};

    // return i2c_master_transmit(device->i2c_dev_handle,
    //                            buf,
    //                            sizeof(buf),
    //                            1000);

    esp_err_t ret = i2c_master_transmit(device->i2c_dev_handle,
                                        buf,
                                        sizeof(buf),
                                        1000);
    if (ret != ESP_OK) {
        printf("MPU6050 WRITE ERROR: reg=0x%02X val=0x%02X err=%s (0x%X)\n",
               reg_to_write, val_to_write, esp_err_to_name(ret), ret);
    }
    return ret;
}


esp_err_t mpu6050_read_reg(struct i2c_device *device, uint8_t reg_to_read, uint8_t *val_to_read, size_t val_size)
{

    uint8_t reg = reg_to_read;
    // return i2c_master_transmit_receive(device->i2c_dev_handle,
    //                                    &reg,
    //                                    1,
    //                                    val_to_read,
    //                                    val_size,
    //                                    1000);

    esp_err_t ret = i2c_master_transmit_receive(device->i2c_dev_handle,
                                                &reg,
                                                1,
                                                val_to_read,
                                                val_size,
                                                4000);
    if (ret != ESP_OK) {
        printf("MPU6050 READ ERROR: reg=0x%02X size=%d err=%s (0x%X)\n",
               reg_to_read, val_size, esp_err_to_name(ret), ret);
    }
    return ret;
}

void print_acc(const Three_Axis_final_t *ax)
{

    if (ax == NULL)
    {
        printf("ACCELERATION NULL\n");
        return;
    }

    printf("ACCEL --- X: %f  Y: %f  Z: %f\n", ax->a_x, ax->a_y, ax->a_z);
}

void print_gyro(const Gyro_Axis_final_t *gyro)
{

    if (gyro == NULL)
    {
        printf("GYROSCOPE NULL\n");
        return;
    }

    printf("GYRO --- X: %f  Y: %f  Z: %f\n", gyro->g_x, gyro->g_y, gyro->g_z);
}

void read_sample_ACC(Three_Axis_t *ax, Three_Axis_final_t *f_ax, uint8_t *r_buff, const int i)
{

    if (ax == NULL || r_buff == NULL || f_ax == NULL)
    {
        return;
    }

    ax->a_x = (int16_t)(r_buff[i + 0] << 8) | r_buff[i + 1];
    ax->a_y = (int16_t)(r_buff[i + 2] << 8) | r_buff[i + 3];
    ax->a_z = (int16_t)(r_buff[i + 4] << 8) | r_buff[i + 5];

    // normalization from row (LSB) to g
    f_ax->a_x = (float)ax->a_x / M_REST;
    f_ax->a_y = (float)ax->a_y / M_REST;
    f_ax->a_z = (float)ax->a_z / M_REST;
}

void read_sample_GYRO(Gyro_Axis_t *gyro, Gyro_Axis_final_t *f_gyro, uint8_t *r_buff, int i)
{

    if (gyro == NULL || r_buff == NULL || f_gyro == NULL)
    {
        return;
    }

    gyro->g_x = (int16_t)(r_buff[i + 0] << 8) | r_buff[i + 1];
    gyro->g_y = (int16_t)(r_buff[i + 2] << 8) | r_buff[i + 3];
    gyro->g_z = (int16_t)(r_buff[i + 4] << 8) | r_buff[i + 5];

    //normalization to °/s
    f_gyro->g_x = (float)(gyro->g_x / SENS_GYRO_RANGE);
    f_gyro->g_y = (float)(gyro->g_y / SENS_GYRO_RANGE);
    f_gyro->g_z = (float)(gyro->g_z / SENS_GYRO_RANGE);

    // orient is degrees
    orient.pitch += f_gyro->g_y * DT;
    orient.yaw   += f_gyro->g_z * DT;

    // from ° to rad
    float pitch = orient.pitch * DEG_TO_RAD;
    float yaw   = orient.yaw   * DEG_TO_RAD;

    Vec3_t v;
    v.x = cosf(pitch) * cosf(yaw);
    v.y = cosf(pitch) * sinf(yaw);
    v.z = sinf(pitch);

    Gyro_Axis_final_t tmp = {v.x, v.y, v.z};

    /* if (ble_manager_is_connected())
    {
        ble_manager_notify_gyro(
            ble_manager_get_conn_handle(),
            &tmp);
    } */
}

esp_err_t empty_FIFO(struct i2c_device *device, Three_Axis_t *axis, Three_Axis_final_t *f_ax, Gyro_Axis_t *gyro, Gyro_Axis_final_t *f_gyro, uint8_t *reading_buffer, int fs)
{

    // ! read data from FIFO and write them in reading_buffer
    if (mpu6050_read_reg(device, MPU6050_FIFO_DATA_R_W, reading_buffer, fs) != ESP_OK)
    {
        return ERR;
    }

    for (uint16_t i = 0; i + 11 < fs; i += 12)
    {

        read_sample_ACC(axis, f_ax, reading_buffer, i);
        read_sample_GYRO(gyro, f_gyro, reading_buffer, i + 6);

        motion_analysis(axis, f_gyro);
    }

    return ESP_OK;
}

esp_err_t mpu6050_read_FIFO(struct i2c_device *device, Three_Axis_t *axis, Gyro_Axis_t *gyro, Three_Axis_final_t *f_ax, Gyro_Axis_final_t *f_gyro)
{

    if (device == NULL)
    {
        return ERR;
    }

    uint8_t fifo_h = 0;
    uint8_t fifo_l = 0;
    uint16_t fifo_size = 0;
    uint8_t reg_int_status = 0;

    // // before read or write on FIFO reset it to clear it up from old data
    // if (set_USR_CTRL(device) != ESP_OK)
    // {
    //     return ERR;
    // }

    // reset at the beginning
    if (!fifo_initialized) {
        if (set_USR_CTRL(device) != ESP_OK) return ERR;
        fifo_initialized = true;
    }
    // time to fill the FIFO up
    vTaskDelay(DELAY_20);

    // read fifo size obtained by a logic OR of: MPU6050_FIFO_COUNT_H 00000000 | 00000000 MPU6050_FIFO_COUNT_L
    if (mpu6050_read_reg(device, MPU6050_FIFO_COUNT_H, &fifo_h, 1) != ESP_OK)
    {
        return ERR;
    }
    if (mpu6050_read_reg(device, MPU6050_FIFO_COUNT_L, &fifo_l, 1) != ESP_OK)
    {
        return ERR;
    }

    fifo_size = ((uint16_t)fifo_h << 8) | fifo_l;

    ESP_LOGI("MPU6050_FIFO", "FIFO SIZE: %d", fifo_size);
    // if not enough OR nothing to read I consider the FIFO as EMPTY
    if (fifo_size < 12 || fifo_size == 0)
    {
        return FIFO_EMPTY;
    }

    // FIRST ALTERNATIVE
    // 1024 is the FIFO MAX_SIZE so if greater than 1024 set it to MAX_SIZE
    // BUT in this way i can lose some data or having incomplete data
    // if(fifo_size > 1024) {
    //     fifo_size = 1024;
    // }
    // SECOND ALTERNATIVE:
    // is to reset the FIFO: the reset will do the next time I will try to read the FIFO
    if (fifo_size > 1024)
    {
        return RESET_FIFO;
    }

    uint8_t reading_buffer[fifo_size];

    if (mpu6050_read_reg(device, MPU6050_INT_STATUS, &reg_int_status, 1) != ESP_OK)
    {
        return ERR;
    }

    if (FIFO_OVERFLOW(reg_int_status))
    {
        // ! if OVERFLOW read all data in FIFO and analyze them
        // if (empty_FIFO(device, axis, f_ax, gyro, f_gyro, reading_buffer, fifo_size) != ESP_OK)
        // {
        //     return ERR;
        // }
        if (set_USR_CTRL(device) != ESP_OK)
        {
            return ERR;
        }
    }

    if (empty_FIFO(device, axis, f_ax, gyro, f_gyro, reading_buffer, fifo_size) != ESP_OK)
    {
        return ERR;
    }

    
    
    return ESP_OK;
}

esp_err_t set_USR_CTRL(struct i2c_device *device)
{

    /*
        set USR_CTRL register to:

        7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
        -----------------------------
        0 | 1 | 0 | 0 | 0 | 1 | 0 | 0
            ^               ^
            | FIFO_EN       | FIFO_RESET
    */
    return mpu6050_write_reg(device,
                             MPU6050_USER_CTRL,
                             0x00 | USER_CTRL_BIT_FIFO_RST | USER_CTRL_BIT_FIFO_EN);
}

esp_err_t set_FIFO_EN(struct i2c_device *device)
{
    /*
        set FIFO_EN register to:

        7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
        -----------------------------
        0 | 1 | 1 | 1 | 1 | 0 | 0 | 0
            ^    ^   ^   ^
            |    |   |   | ACCEL_FIFO_EN
            |    |   | ZG_FIFO_EN
            |    | YG_FIFO_EN
            | XG_FIFO_EN

    */
    // write of all the sensors data in the FIFO

    return mpu6050_write_reg(device,
                             MPU6050_FIFO_EN,
                             FIFO_EN_BIT_ACCEL | FIFO_EN_BIT_XG | FIFO_EN_BIT_YG | FIFO_EN_BIT_ZG);
}

esp_err_t set_FIFO_INT(struct i2c_device *device)
{
    /*
        set INT_ENABLE register to:

        7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
        -----------------------------
        0 | 0 | 0 | 1 | 0 | 0 | 0 | 0
                    ^
                    |
                    | FIFO_OFLOW_EN
    */
    return mpu6050_write_reg(device,
                             MPU6050_INT_ENABLE,
                             MPU6050_INT_ENABLE_BIT_FIFO_OFLOW_INT);
}

esp_err_t acc_config(struct i2c_device *device)
{

    if (device == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // sensor wake up
    if (mpu6050_write_reg(device, PWR_MGMT_1, PWR_MGMT_1_CONFIG) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }
    vTaskDelay(DELAY_10);
    // Sample rate = 1kHz / (1 + 7) = 125 Hz
    if (mpu6050_write_reg(device, SMPLRT_DIV, 0x07) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }
    // Low-pass filter 5Hz (0x06 è ok)
    if (mpu6050_write_reg(device, CONFIG, 0x06) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
        config g_range
    */
    if (mpu6050_write_reg(device, MPU6050_ACCEL_CONFIG, MPU6050_ACC_G_RANGE) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
        config full_range 250
        FS_SELF = 0 dec
    */
    if (mpu6050_write_reg(device, MPU6050_GYRO_CONFIG, MPU6050_GYRO_RANGE) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (set_USR_CTRL(device) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }

    vTaskDelay(DELAY_20);

    if (set_FIFO_INT(device) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (set_FIFO_EN(device) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

bool verify_step(const Three_Axis_t *ax)
{

    if (ax == NULL)
    {
        return false;
    }

    float M = sqrt((ax->a_x * ax->a_x) +
                   (ax->a_y * ax->a_y) +
                   (ax->a_z * ax->a_z));

    static bool up = false;

    if (!up && M > (M_REST + THRESHOLD_H))
    { // rising edge
        // ! this means that one step is detected when M raises above TH_H and up is false (down)
        up = true;
        return true;
    }
    else if (up && M < (M_REST + THRESHOLD_L))
    { // falling edge
        up = false;
    }

    return false;
}

bool verify_wrist_rotation(const Gyro_Axis_final_t *g)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // refractory window
    // avoid double triggers
    if ((now - rotation.last_trigger) < REFRACT_MS) {
        return false;
    }

    // angular velocity °/s
    float w = fabsf(g->g_z);   // wrist rotation axis

    // noise threshold
    if (w < MIN_ROT_ANGLE) {
        // leaky integrator to forget noise
        rotation.integrated_angle *= 0.95f;
        return false;
    }

    rotation.integrated_angle += w * DT;   // °/s * s -> °

    if (rotation.integrated_angle >= WRIST_ROT_THRESHOLD) {
        rotation.integrated_angle = 0.0f;
        rotation.last_trigger = now;
        return true;
    }

    return false;
}

void motion_analysis(const Three_Axis_t *ax, const Gyro_Axis_final_t *gyro)
{

    bool step = verify_step(ax);
    bool wrist = verify_wrist_rotation(gyro);

    if (step && !wrist)
    {
        global_parameters.step_cntr ++;
        printf("STEPS: %d\n", global_parameters.step_cntr);
        fflush(stdout);
    }
    else if (wrist)
    {
        printf("WRIST ROTATION DETECT\n");
        fflush(stdout);

        //EventType evt = EVT_GYRO;
        //xQueueSend(event_queue, &evt, 0);
    }
}

