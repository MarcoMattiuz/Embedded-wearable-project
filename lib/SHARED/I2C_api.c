#include "I2C_api.h"


// One mutex per I2C bus. Created lazily in the init functions.
static SemaphoreHandle_t i2c_mutex_0 = NULL;
static SemaphoreHandle_t i2c_mutex_1 = NULL;

SemaphoreHandle_t i2c_get_mutex_port0(void) { return i2c_mutex_0; }
SemaphoreHandle_t i2c_get_mutex_port1(void) { return i2c_mutex_1; }



void init_I2C_bus_PORT0(i2c_master_bus_handle_t* i2c_bus){
    if (i2c_mutex_0 == NULL) {
        i2c_mutex_0 = xSemaphoreCreateMutex();
        if (i2c_mutex_0 == NULL) {
            printf("Failed to create I2C mutex for PORT0\n");
            abort();
        }
    }
    esp_err_t esp_ret;
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT_0,
        .sda_io_num = MAX30102_I2C_SDA_PIN,
        .scl_io_num = MAX30102_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = I2C_GLITCH_IGNORE_CNT,
        .flags.enable_internal_pullup = 1
    };

    // Initialize I2C bus
    esp_ret = i2c_new_master_bus(&bus_config, i2c_bus);
    if (esp_ret != ESP_OK) {
        printf("Failed to create I2C master bus: %d\n", esp_ret);
        abort();
    }
}

void init_I2C_bus_PORT1(i2c_master_bus_handle_t* i2c_bus){
    if (i2c_mutex_1 == NULL) {
        i2c_mutex_1 = xSemaphoreCreateMutex();
        if (i2c_mutex_1 == NULL) {
            printf("Failed to create I2C mutex for PORT1\n");
            abort();
        }
    }
    esp_err_t esp_ret;
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT_1,
        .sda_io_num = MPU6050_I2C_SDA_PIN,
        .scl_io_num = MPU6050_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = I2C_GLITCH_IGNORE_CNT,
        .flags.enable_internal_pullup = 1
    };

    // Initialize I2C bus
    esp_ret = i2c_new_master_bus(&bus_config, i2c_bus);
    if (esp_ret != ESP_OK) {
        printf("Failed to create I2C master bus: %d\n", esp_ret);
        abort();
    }
}