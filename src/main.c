#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "MPU6050_api.h"
#include "I2C_api.h"
#include "interrupt_api.h"
#include "reg.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) {

    interrupt_setup_init();

    i2c_init();
    //i2c_scan();

    xTaskCreate(
        task_acc,
        "task_acc_debug",
        4096,
        NULL, 
        1,
        NULL
    );
}