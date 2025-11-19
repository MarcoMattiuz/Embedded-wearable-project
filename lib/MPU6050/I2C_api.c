#include "I2C_api.h" 
#include <stdio.h>
#include "reg.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"  

/*void i2c_scan() {
    printf("Scanning I2C bus...\n");
    for (uint8_t addr = 1; addr < 0x7F; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
        if (ret == ESP_OK) {
            printf("Device found at 0x%02X\n", addr);
        }

        i2c_cmd_link_delete(cmd);
    }
    printf("Scan done.\n");
}*/

void i2c_init() {

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,             //esp32 will be the master
        .sda_io_num = I2C_SDA_PIN,           //set sda 
        .scl_io_num = I2C_SCL_PIN,           //set scl
        .sda_pullup_en = GPIO_PULLUP_ENABLE, //def UP
        .scl_pullup_en = GPIO_PULLUP_ENABLE, //def UP
        .master.clk_speed = I2C_FREQ_HZ      //set bus speed
    };
    i2c_param_config(I2C_MASTER_NUM, &i2c_conf);
    i2c_driver_install(I2C_MASTER_NUM, i2c_conf.mode, 0, 0, 0);
}