#ifndef __I2C_API_H__
#define __I2C_API_H__

#include <stdio.h>
#include "reg.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"     

//void i2c_scan();
void i2c_init();

#endif