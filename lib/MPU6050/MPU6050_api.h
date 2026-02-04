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
#include "../SHARED/global_param.h"
#include "../sh1106/display_fsm.h"
#include "../BLE/ble_manager.h"

static bool fifo_initialized = false;

esp_err_t mpu6050_write_reg (struct i2c_device* device, uint8_t reg_to_write, uint8_t val_to_write);
esp_err_t mpu6050_read_reg  (struct i2c_device* device, uint8_t reg_to_read, uint8_t* val_to_read, size_t val_size);

void print_acc  (const Three_Axis_final_t* ax);
void print_gyro (const Gyro_Axis_final_t* gyro);

void read_sample_ACC  (Three_Axis_t* ax, Three_Axis_final_t* f_ax, uint8_t* r_buff, int i);
void read_sample_GYRO (Gyro_Axis_t* gyro, Gyro_Axis_final_t* f_gyro, uint8_t* r_buff, int i);

esp_err_t empty_FIFO        (struct i2c_device* device, Three_Axis_t *axis, Three_Axis_final_t* f_ax, Gyro_Axis_t* gyro, Gyro_Axis_final_t* f_gyro, uint8_t* reading_buffer, int fs);
esp_err_t mpu6050_read_FIFO (struct i2c_device* device, Three_Axis_t* axis, Gyro_Axis_t* gyro, Three_Axis_final_t* f_ax, Gyro_Axis_final_t* f_gyro);

esp_err_t set_USR_CTRL (struct i2c_device* device);
esp_err_t set_FIFO_EN  (struct i2c_device* device);
esp_err_t set_FIFO_INT (struct i2c_device* device);
esp_err_t acc_config   (struct i2c_device* device);

float low_pass_filter_M     (const float M);
bool  verify_step           (const Three_Axis_final_t* ax);
bool  verify_wrist_rotation (const Gyro_Axis_final_t* g);
void  motion_analysis       (const Three_Axis_final_t* ax, const Gyro_Axis_final_t* gyro);

void send_gyro_data_debug (void* pvParameters);

#endif