/********************************************************************************************
 * Project: MPU6050 ESP32 Sensor Interface
 * Author: Muhammad Idrees
 * 
 * Description:
 * This source file implements the functions required to interface with the MPU6050 sensor
 * using the ESP32. It handles sensor initialization, data reading, and conversion to 
 * physical units, along with calibration functions to correct sensor biases.
 * 
 * Author's Background:
 * Name: Muhammad Idrees
 * Degree: Bachelor's in Electrical and Electronics Engineering
 * Institution: Institute of Space Technology, Islamabad
 * 
 * License:
 * This code is created solely by Muhammad Idrees for educational and research purposes.
 * Redistribution and use in source and binary forms, with or without modification, are 
 * permitted provided that the above author information and this permission notice appear 
 * in all copies.
 * 
 * Key Features:
 * - I2C communication setup for MPU6050.
 * - Raw data acquisition and conversion.
 * - Calibration functions for accurate readings.
 * 
 * Date: [28/7/24]
 ********************************************************************************************/




/// mpu6050.c code starts
#include "mpu6050.h"


#define ACCEL_SCALE 16384.0f // for ±2g range
#define GYRO_SCALE 131.0f // for ±250°/s range
#define GRAVITY 9.8f // m/s²

static float accel_bias[3] = {1.12f, -0.30f, -0.55f}; // Biases for accel X, Y, Z
static float gyro_bias[3] = {-0.27f, 0.24f, 0.16f}; // Biases for gyro X, Y, Z

static i2c_master_bus_handle_t i2c_bus = NULL;
i2c_master_dev_handle_t mpu6050_dev = NULL;


void mpu6050_set_handle(i2c_master_dev_handle_t dev)
{
    mpu6050_dev = dev;
}


esp_err_t mpu6050_init(gpio_num_t sda, gpio_num_t scl)
{
    esp_err_t ret;

    /* Create I2C bus */
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ret = i2c_new_master_bus(&bus_cfg, &i2c_bus);
    if (ret != ESP_OK)
        return ret;

    /* Add MPU6050 device */
    i2c_device_config_t dev_cfg = {
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = 400000,
    };

    ret = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &mpu6050_dev);
    if (ret != ESP_OK)
        return ret;

    /* Wake up MPU6050 */
    uint8_t cmd[2] = {
        MPU6050_REG_PWR_MGMT_1,
        0x00};

    return i2c_master_transmit(mpu6050_dev, cmd, sizeof(cmd), -1);
}

esp_err_t mpu6050_read_raw_data(int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                                int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z)
{
    uint8_t reg = MPU6050_REG_ACCEL_XOUT_H;
    uint8_t data[14];

    esp_err_t ret = i2c_master_transmit_receive(
        mpu6050_dev,
        &reg,
        1,
        data,
        sizeof(data),
        -1);

    if (ret != ESP_OK)
        return ret;

    *accel_x = (data[0] << 8) | data[1];
    *accel_y = (data[2] << 8) | data[3];
    *accel_z = (data[4] << 8) | data[5];

    *gyro_x = (data[8] << 8) | data[9];
    *gyro_y = (data[10] << 8) | data[11];
    *gyro_z = (data[12] << 8) | data[13];

    return ESP_OK;
}

void mpu6050_convert_accel(int16_t raw_x, int16_t raw_y, int16_t raw_z, float *accel_x, float *accel_y, float *accel_z) {
    *accel_x = ((raw_x / ACCEL_SCALE) * GRAVITY) - accel_bias[0];
    *accel_y = ((raw_y / ACCEL_SCALE) * GRAVITY) - accel_bias[1];
    *accel_z = ((raw_z / ACCEL_SCALE) * GRAVITY) - accel_bias[2];
}

void mpu6050_convert_gyro(int16_t raw_x, int16_t raw_y, int16_t raw_z, float *gyro_x, float *gyro_y, float *gyro_z) {
    *gyro_x = (raw_x / GYRO_SCALE) - gyro_bias[0];
    *gyro_y = (raw_y / GYRO_SCALE) - gyro_bias[1];
    *gyro_z = (raw_z / GYRO_SCALE) - gyro_bias[2];
}

void mpu6050_calibrate(float *accel_b, float *gyro_b)
{
    int16_t ax, ay, az, gx, gy, gz;
    float ax_g, ay_g, az_g, gx_d, gy_d, gz_d;

    float ax_s = 0, ay_s = 0, az_s = 0;
    float gx_s = 0, gy_s = 0, gz_s = 0;

    const int samples = 100;

    for (int i = 0; i < samples; i++)
    {
        mpu6050_read_raw_data(&ax, &ay, &az, &gx, &gy, &gz);

        mpu6050_convert_accel(ax, ay, az, &ax_g, &ay_g, &az_g);
        mpu6050_convert_gyro(gx, gy, gz, &gx_d, &gy_d, &gz_d);

        ax_s += ax_g;
        ay_s += ay_g;
        az_s += az_g;

        gx_s += gx_d;
        gy_s += gy_d;
        gz_s += gz_d;
    }

    accel_b[0] = ax_s / samples;
    accel_b[1] = ay_s / samples;
    accel_b[2] = (az_s / samples) - GRAVITY;

    gyro_b[0] = gx_s / samples;
    gyro_b[1] = gy_s / samples;
    gyro_b[2] = gz_s / samples;
}
