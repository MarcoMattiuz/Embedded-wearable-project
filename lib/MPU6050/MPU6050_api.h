#ifndef __MPU6050_API_H__
#define __MPU6050_API_H__

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"  
#include "driver/i2c_master.h"   
#include "esp_err.h"
#include "reg.h"
#include "macros.h"
#include "I2C_api.h"

/*
    ! return ESP_ERR_INVALID_ARG in this part og the project is considered as STANDARD ERROR
*/
esp_err_t mpu6050_write_reg (struct i2c_device* device, uint8_t reg_to_write, uint8_t val_to_write);
esp_err_t mpu6050_read_reg  (struct i2c_device* device, uint8_t reg_to_read, uint8_t* val_to_read, size_t val_size);

void      print_acc         (const Three_Axis_t* ax);
void      read_sample       (Three_Axis_t* ax, const uint8_t* r_buff);
esp_err_t mpu6050_read_ACC  (struct i2c_device* device, Three_Axis_t* axis);

esp_err_t set_USR_CTRL      (struct i2c_device* device);
esp_err_t set_FIFO_EN       (struct i2c_device* device);
esp_err_t acc_config        (struct i2c_device* device);

int       low_pass_filter   (const int M);
bool      verify_step       (const Three_Axis_t* ax);
void      step_counter      (const Three_Axis_t* ax);

bool      wrist_detection   (const Three_Axis_t* ax);

void      task_acc          (void* pvParameters);
void      task_gyro         (void* pvParameters);

#endif