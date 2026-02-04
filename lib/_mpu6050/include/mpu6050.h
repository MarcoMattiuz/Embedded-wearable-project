/********************************************************************************************
 * Project: MPU6050 ESP32 Sensor Interface
 * Author: Muhammad Idrees
 *
 * Description:
 * This header file defines the interface for the MPU6050 sensor interactions. It includes
 * function prototypes for initializing the sensor, reading data, and processing the data
 * into meaningful physical values.
 *
 * Author's Background:
 * Name: Muhammad Idrees
 * Degree: Bachelor's in Electrical and Electronics Engineering
 * Institution: Institute of Space Technology, Islamabad
 *
 * License:
 * Authored by Muhammad Idrees, this header file accompanies the MPU6050 interface library.
 * Use of this file is encouraged for educational and development purposes, acknowledging
 * the author's work.
 *
 * Key Features:
 * - I2C communication protocol definitions.
 * - Sensor data processing function prototypes.
 *
 * Date: [28/7/2024]
 ********************************************************************************************/

#ifndef MPU6050_H
#define MPU6050_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#define MPU6050_ADDR 0x68

// MPU6050 Register Addresses
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H 0x43

extern i2c_master_dev_handle_t mpu6050_dev;

// Function prototypes
esp_err_t mpu6050_init(gpio_num_t sda, gpio_num_t scl);
esp_err_t mpu6050_read_raw_data(int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                                    int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z);
void mpu6050_convert_accel(int16_t raw_x, int16_t raw_y, int16_t raw_z, float *accel_x, float *accel_y, float *accel_z);
void mpu6050_convert_gyro(int16_t raw_x, int16_t raw_y, int16_t raw_z, float *gyro_x, float *gyro_y, float *gyro_z);
void mpu6050_calibrate(float *accel_b, float *gyro_b);

    // Roll and pitch calculation functions
void roll_pitch_init(void);
void roll_pitch_update(float accel_x, float accel_y, float accel_z, float gyro_x, float gyro_y, float gyro_z);
float roll_get(void);
float pitch_get(void);
void mpu6050_set_handle(i2c_master_dev_handle_t dev);

#endif // MPU6050_H
