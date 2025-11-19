#include "I2C_api.h" 


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

// void i2c_init() {

//     i2c_config_t i2c_conf = {
//         .mode = I2C_MODE_MASTER,             //esp32 will be the master
//         .sda_io_num = I2C_SDA_PIN,           //set sda 
//         .scl_io_num = I2C_SCL_PIN,           //set scl
//         .sda_pullup_en = GPIO_PULLUP_ENABLE, //def UP
//         .scl_pullup_en = GPIO_PULLUP_ENABLE, //def UP
//         .master.clk_speed = I2C_FREQ_HZ      //set bus speed
//     };
//     i2c_param_config(I2C_MASTER_NUM, &i2c_conf);
//     i2c_driver_install(I2C_MASTER_NUM, i2c_conf.mode, 0, 0, 0);
// }

void init_I2C_bus_PORT0(i2c_master_bus_handle_t *i2c_bus){
    esp_err_t esp_ret;
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
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