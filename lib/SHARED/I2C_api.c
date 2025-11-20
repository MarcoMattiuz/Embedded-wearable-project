#include "I2C_api.h" 

void init_I2C_bus_PORT0(i2c_master_bus_handle_t* i2c_bus){
   
    esp_err_t esp_ret;
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT_0,
        .sda_io_num = MAX30102_I2C_SDA_PIN,
        .scl_io_num = MAX30102_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = I2C_GLITCH_IGNORE_CNT,
        .flags.enable_internal_pullup = 1,
    };

    // Initialize I2C bus
    esp_ret = i2c_new_master_bus(&bus_config, i2c_bus);
    if (esp_ret != ESP_OK) {
        printf("Failed to create I2C master bus: %d\n", esp_ret);
        abort();
    }
}

void init_I2C_bus_PORT1(i2c_master_bus_handle_t* i2c_bus){
   
    esp_err_t esp_ret;
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT_1,
        .sda_io_num = MPU6050_I2C_SDA_PIN,
        .scl_io_num = MPU6050_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = I2C_GLITCH_IGNORE_CNT,
        .flags.enable_internal_pullup = 1,
    };

    // Initialize I2C bus
    esp_ret = i2c_new_master_bus(&bus_config, i2c_bus);
    if (esp_ret != ESP_OK) {
        printf("Failed to create I2C master bus: %d\n", esp_ret);
        abort();
    }
}