#ifndef __MPU6050_API_H__
#define __MPU6050_API_H__

#include <stdio.h>
#include "reg.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include <stdbool.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"  

esp_err_t mpu6050_write_reg (uint8_t reg, uint8_t data);
esp_err_t mpu6050_read_reg  (uint8_t reg, uint8_t *data, size_t len);
void      read_sample       (Three_Axis_t* ax, const Reading_Buffer_t r_buff);
void      print_acc         (const Three_Axis_t* ax);
void      acc_config        ();
int       low_pass_filter   (const int M);
bool      verify_step       (const Three_Axis_t* ax);
void      step_counter      (const Three_Axis_t* ax);
void      send_interrupt    ();
bool      wrist_detection   (const Three_Axis_t* ax);
void      task_acc          (void* pvParameters);

#endif