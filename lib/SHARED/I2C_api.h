#ifndef __I2C_API_H__
#define __I2C_API_H__

#include <stdio.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"     
#include "macros.h"
#include <stdlib.h>
#include <string.h>
#include "freertos/semphr.h"

/*
    ! i2c_device structure
*/
struct i2c_device {
    //Read function pointer
    i2c_master_dev_handle_t i2c_dev_handle;
    i2c_device_config_t     i2c_dev_config;
};

void init_I2C_bus_PORT0 (i2c_master_bus_handle_t* i2c_bus);
void init_I2C_bus_PORT1 (i2c_master_bus_handle_t* i2c_bus);

SemaphoreHandle_t i2c_get_mutex_port0(void);
SemaphoreHandle_t i2c_get_mutex_port1(void);

#define I2C_LOCK_0()   xSemaphoreTake(i2c_get_mutex_port0(), portMAX_DELAY)
#define I2C_UNLOCK_0() xSemaphoreGive(i2c_get_mutex_port0())
#define I2C_LOCK_1()   xSemaphoreTake(i2c_get_mutex_port1(), portMAX_DELAY)
#define I2C_UNLOCK_1() xSemaphoreGive(i2c_get_mutex_port1())

esp_err_t add_device_ENS160(void);

#endif