#include "../include/mpu6050.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static float accel_bias[3] = {1.12f, -0.30f, -0.55f};
static float gyro_bias[3]  = {-0.27f, 0.24f, 0.16f};

// Handle for the new I2C device
static i2c_master_dev_handle_t mpu6050_dev = NULL;

// I2C communication setup for MPU6050
void mpu6050_set_handle(i2c_master_dev_handle_t dev_handle)
{
    mpu6050_dev = dev_handle;
}

/*
    CONFIGURE MPU6050 SENSOR
*/
esp_err_t mpu6050_init()
{
    esp_err_t ret;

    uint8_t cmd[2];

    // Wake up MPU6050 (write 0 to PWR_MGMT_1)
    cmd[0] = MPU6050_REG_PWR_MGMT_1;
    cmd[1] = 0x00;
    ret = i2c_master_transmit(mpu6050_dev, cmd, sizeof(cmd), -1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    // Sample rate = 1kHz / (1 + 7) = 125 Hz
    cmd[0] = SMPLRT_DIV;
    cmd[1] = 0x07;
    ret = i2c_master_transmit(mpu6050_dev, cmd, sizeof(cmd), -1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    // Configure low pass filter to 5Hz
    cmd[0] = CONFIG;
    cmd[1] = 0x06;
    ret = i2c_master_transmit(mpu6050_dev, cmd, sizeof(cmd), -1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    // Configure accel ±2g
    cmd[0] = ACCEL_CONFIG;
    cmd[1] = ACCEL_G_RANGE;
    ret = i2c_master_transmit(mpu6050_dev, cmd, sizeof(cmd), -1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    // Configure gyro ±250°/s
    cmd[0] = GYRO_CONFIG;
    cmd[1] = GYRO_RANGE;
    ret = i2c_master_transmit(mpu6050_dev, cmd, sizeof(cmd), -1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    // Enable FIFO for accel and gyro
    cmd[0] = USER_CTRL;
    cmd[1] = 0x40;
    ret = i2c_master_transmit(mpu6050_dev, cmd, sizeof(cmd), -1);
    if (ret != ESP_OK)
    {
        return ret;
    }   

    // Enable OVERFLOW interrupt
    cmd[0] = INT_ENABLE;
    cmd[1] = 0x80 | 0x08 | 0x40 | 0x20 | 0x10;
    ret = i2c_master_transmit(mpu6050_dev, cmd, sizeof(cmd), -1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    return ESP_OK;
}

/*
    ACQUIRE RAW DATA FROM MPU6050
*/
int get_fifo_size(void) {

    uint8_t  fifo_h = 0;
    uint8_t  fifo_l = 0;
    uint16_t fifo_size = 0; // byte in a sample
    uint8_t  reg_int_status = 0;

    esp_err_t ret = i2c_master_transmit_receive(mpu6050_dev,
                                                MPU6050_FIFO_COUNT_H,
                                                1,
                                                &fifo_h,
                                                sizeof(fifo_h),
                                                -1);
    if (ret != ESP_OK) {
        ESP_LOGE("mpu6050", "Failed to read: %s", esp_err_to_name(ret));
    }
    ret = i2c_master_transmit_receive(mpu6050_dev,
                                      MPU6050_FIFO_COUNT_L,
                                      1,
                                      &fifo_l,
                                      sizeof(fifo_l),
                                      -1);
    if (ret != ESP_OK) 
    {
        return -3;
    }

    fifo_size = (fifo_h << 8) | fifo_l;
    ESP_LOGI("MPU6050_FIFO", "FIFO SIZE: %d", fifo_size);
    if (fifo_size < FIFO_SAMPLE_SIZE || fifo_size == 0)
    {
        return -1;
    }
    if (fifo_size % FIFO_SAMPLE_SIZE != 0)
    {
        return -2;
    }

    return fifo_size;
}

esp_err_t mpu6050_read_raw_data(Raw_Data_acc *raw_acc, Raw_Data_gyro *raw_gyro)
{
    esp_err_t ret;

    // REG DATA: LIVE DATA
    // uint8_t data[14];
    // ret = i2c_master_transmit_receive(mpu6050_dev,
    //                                             MPU6050_REG_ACCEL_XOUT_H,
    //                                             1,
    //                                             data,
    //                                             sizeof(data),
    //                                             -1);
    
    // FIFO DATA: HISTORICAL DATA
    uint8_t data[FIFO_SAMPLE_SIZE];

    ret = i2c_master_transmit_receive(mpu6050_dev,
                                      MPU6050_FIFO_DATA_R_W,
                                      1,
                                      data,
                                      sizeof(data),
                                      -1);

    if (ret != ESP_OK)
    {
        ESP_LOGE("mpu6050", "Failed to read: %s", esp_err_to_name(ret));
    }

    raw_acc->a_x  = (data[0] << 8) | data[1];
    raw_acc->a_y  = (data[2] << 8) | data[3];
    raw_acc->a_z  = (data[4] << 8) | data[5];
    ESP_LOGI("MPU6050_FIFO", "RAW ACCEL: X=%d Y=%d Z=%d", raw_acc->a_x, raw_acc->a_y, raw_acc->a_z);

    /*
        raw_temp = (data[6] << 8) | data[7];
        I dont use it
    */

    raw_gyro->g_x = (data[8]  << 8) | data[9];
    raw_gyro->g_y = (data[10] << 8) | data[11];
    raw_gyro->g_z = (data[12] << 8) | data[13];
    ESP_LOGI("MPU6050_FIFO", "RAW GYRO:  X=%d Y=%d Z=%d", raw_gyro->g_x, raw_gyro->g_y, raw_gyro->g_z);

    return ESP_OK;
}

/*
    CONVERT RAW DATA TO PHYSICAL UNIT

    ACC :  g -> m/s²  
    GYRO:  °/s    
*/
void mpu6050_convert_accel(Raw_Data_acc *raw_acc, ACC_Three_Axis_t *acc_data)
{
    acc_data->a_x = ((raw_acc->a_x / ACCEL_SCALE) * GRAVITY) - accel_bias[0];
    acc_data->a_y = ((raw_acc->a_y / ACCEL_SCALE) * GRAVITY) - accel_bias[1];
    acc_data->a_z = ((raw_acc->a_z / ACCEL_SCALE) * GRAVITY) - accel_bias[2];
}

void mpu6050_convert_gyro(Raw_Data_gyro *raw_gyro, GYRO_Three_Axis_t *gyro_data)
{
    gyro_data->g_x = (raw_gyro->g_x / GYRO_SCALE) - gyro_bias[0];
    gyro_data->g_y = (raw_gyro->g_y / GYRO_SCALE) - gyro_bias[1];
    gyro_data->g_z = (raw_gyro->g_z / GYRO_SCALE) - gyro_bias[2];
}

/*
    CALIBRATION FUNCTION
*/
void mpu6050_calibrate(float *accel_bias_out, float *gyro_bias_out)
{

     /*
        The vars *_sum are used to accumulate measurements
        across multiple samples to calculate the average, which allows us to estimate
        the sensor bias while reducing the influence of noise.
        In the case of the Z-axis accelerometer, gravity is subtracted to
        isolate the actual offset.
   */

    Raw_Data_acc     raw_acc  = {0, 0, 0};
    ACC_Three_Axis_t acc_data = {0.0f, 0.0f, 0.0f};
    ACC_accumulated  acc_sum  = {0.0f, 0.0f, 0.0f};
    
    Raw_Data_gyro     raw_gyro  = {0, 0, 0};
    GYRO_Three_Axis_t gyro_data = {0.0f, 0.0f, 0.0f};
    GYRO_accumulated  gyro_sum  = {0.0f, 0.0f, 0.0f};

    int target_samples = 100;
    int collected      = 0;
    int fifo_size      = get_fifo_size();

    while (collected < target_samples) 
    {
        if (fifo_size == -1)
        {
            ESP_LOGE("MPU6050_CALIB", "Not enough data in FIFO!");
            continue;
        } 
        else if (fifo_size == -2)
        {
            ESP_LOGE("MPU6050_CALIB", "Data misalignment detected!");
            continue;
        } 
        else if (fifo_size == -3)
        {
            ESP_LOGE("MPU6050_CALIB", "Failed to read!");
            continue;
        }

        if (mpu6050_read_raw_data(&raw_acc, &raw_gyro) != ESP_OK)
        {
            continue;
        }

        int samples_in_fifo = fifo_size / FIFO_SAMPLE_SIZE;

        for (int i = 0; i < samples_in_fifo && collected < target_samples; i++)
        {
            if (mpu6050_read_raw_data(&raw_acc, &raw_gyro) != ESP_OK)
            {
                continue;
            }

            mpu6050_convert_accel(&raw_acc, &acc_data);
            mpu6050_convert_gyro(&raw_gyro, &gyro_data);

            acc_sum.a_x_sum += acc_data.a_x;
            acc_sum.a_y_sum += acc_data.a_y;
            acc_sum.a_z_sum += acc_data.a_z;
            gyro_sum.g_x_sum += gyro_data.g_x;
            gyro_sum.g_y_sum += gyro_data.g_y;
            gyro_sum.g_z_sum += gyro_data.g_z;
            
            collected++;
        }
    }

    // for (int i = 0; i < samples; i++)
    // {
    //     if (mpu6050_read_raw_data(&raw_acc, &raw_gyro) != ESP_OK)
    //     {
    //         continue;
    //     }

    //     mpu6050_convert_accel(&raw_acc, &acc_data);
    //     mpu6050_convert_gyro(&raw_gyro, &gyro_data);

    //     acc_sum.a_x_sum += acc_data.a_x;
    //     acc_sum.a_y_sum += acc_data.a_y;
    //     acc_sum.a_z_sum += acc_data.a_z;
    //     gyro_sum.g_x_sum += gyro_data.g_x;
    //     gyro_sum.g_y_sum += gyro_data.g_y;
    //     gyro_sum.g_z_sum += gyro_data.g_z;
    // }

    //AVG: rapresent the bias (the value which the sensor reads when it is still)
    accel_bias_out[0] =  acc_sum.a_x_sum / target_samples;
    accel_bias_out[1] =  acc_sum.a_y_sum / target_samples;
    accel_bias_out[2] = (acc_sum.a_z_sum / target_samples) - GRAVITY;

    gyro_bias_out[0] = gyro_sum.g_x_sum / target_samples;
    gyro_bias_out[1] = gyro_sum.g_y_sum / target_samples;
    gyro_bias_out[2] = gyro_sum.g_z_sum / target_samples;
}
