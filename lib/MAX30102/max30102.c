#include "max30102.h"

uint32_t RED_buffer[MAX30102_BPM_SAMPLES_SIZE] = {0};
int RED_buffer_index = 0;
uint32_t IR_buffer[MAX30102_BPM_SAMPLES_SIZE] = {0};
int IR_buffer_index = 0;
int16_t RED_ac_buffer[MAX30102_BPM_SAMPLES_SIZE] = {0};
int16_t IR_ac_buffer[MAX30102_BPM_SAMPLES_SIZE] = {0};
int head_beat_count = 0;




esp_err_t max30102_set_register(struct i2c_device *device, uint8_t reg,uint8_t mode){
    uint8_t txbuf[2];
    txbuf[0] = reg;
    txbuf[1] = mode;
    I2C_LOCK_0();
    esp_err_t esp_ret = i2c_master_transmit(device->i2c_dev_handle, txbuf, sizeof(txbuf), 1000);
    I2C_UNLOCK_0();
    return esp_ret;
}

esp_err_t init_multiled_mode(struct i2c_device *device, uint8_t led_red_power, uint8_t led_ir_power, uint8_t SPO2_config) {
    esp_err_t esp_ret;
    esp_ret = max30102_set_register(device, MAX30102_MODE_CFG_ADDR, MAX30102_RESET);
    if (esp_ret != ESP_OK) {
        return esp_ret;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_ret = max30102_set_register(device, MAX30102_SPO2_CFG_ADDR, SPO2_config);
    if (esp_ret != ESP_OK) {
        return esp_ret;
    }
    esp_ret = max30102_set_register(device, MAX30102_LED1_PA_ADDR, led_red_power); // Ridotto da 0xFF a 0x1F (~6.4mA)
    if (esp_ret != ESP_OK) {
        return esp_ret;
    }
    esp_ret = max30102_set_register(device, MAX30102_LED2_PA_ADDR, led_ir_power); // Ridotto da 0xFF a 0x1F (~6.4mA)
    if (esp_ret != ESP_OK) {
        return esp_ret;
    }
    esp_ret = max30102_set_register(device, MAX30102_SLOT_1_2_ADDR, 0x21);
    if (esp_ret != ESP_OK) {
        return esp_ret;
    }
    esp_ret = max30102_set_register(device, MAX30102_MODE_CFG_ADDR, MAX30102_MULTILED_MODE);
    if (esp_ret != ESP_OK) {
        return esp_ret;
    }
    return esp_ret;
}

esp_err_t init_hr_mode(struct i2c_device *device, uint8_t led_red_power, uint8_t led_ir_power, uint8_t SPO2_config) {
    esp_err_t esp_ret;
    esp_ret = max30102_set_register(device, MAX30102_MODE_CFG_ADDR, MAX30102_RESET);
    if (esp_ret != ESP_OK) {
        return esp_ret;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_ret = max30102_set_register(device, MAX30102_SPO2_CFG_ADDR, SPO2_config);
    if (esp_ret != ESP_OK) {
        return esp_ret;
    }
    esp_ret = max30102_set_register(device, MAX30102_LED1_PA_ADDR, led_red_power); // Ridotto da 0xFF a 0x1F (~6.4mA)
    if (esp_ret != ESP_OK) {
        return esp_ret;
    }
    esp_ret = max30102_set_register(device, MAX30102_LED2_PA_ADDR, led_ir_power); // Ridotto da 0xFF a 0x1F (~6.4mA)
    if (esp_ret != ESP_OK) {
        return esp_ret;
    }
    esp_ret = max30102_set_register(device, MAX30102_MODE_CFG_ADDR, MAX30102_HR_MODE);
    if (esp_ret != ESP_OK) {
        return esp_ret;
    }
    return esp_ret;
}

esp_err_t reset_fifo_registers(struct i2c_device *device) {
    esp_err_t esp_ret;
    esp_ret = max30102_set_register(device, MAX30102_FIFO_WR_PTR_ADDR, 0x00);
    if (esp_ret != ESP_OK) {
        DBG_PRINTF("Failed to reset FIFO write pointer: %d\n", esp_ret);
        abort();
    }
    
    esp_ret = max30102_set_register(device, MAX30102_FIFO_RD_PTR_ADDR, 0x00);
    if (esp_ret != ESP_OK) {
        DBG_PRINTF("Failed to reset FIFO read pointer: %d\n", esp_ret);
        abort();
    }
    
    esp_ret = max30102_set_register(device, MAX30102_FIFO_OVF_CTR_ADDR, 0x00);
    if (esp_ret != ESP_OK) {
        DBG_PRINTF("Failed to reset FIFO overflow counter: %d\n", esp_ret);
        abort();
    }
    // DBG_PRINTF("FIFO pointers reset\n");
    return ESP_OK;
}

static bool update_red_buffers(uint32_t value) {
    RED_buffer[RED_buffer_index] = value;
    RED_ac_buffer[RED_buffer_index] = get_RED_AC(value);
    RED_buffer_index = (RED_buffer_index + 1) % MAX30102_BPM_SAMPLES_SIZE;
    if(RED_buffer_index==MAX30102_BPM_SAMPLES_SIZE-1){
        return true;
    }
    return false;
}

static bool update_ir_buffers(uint32_t value) {
    IR_buffer[IR_buffer_index] = value;
    IR_ac_buffer[IR_buffer_index] = get_IR_AC(value); 
    IR_buffer_index = (IR_buffer_index + 1) % MAX30102_BPM_SAMPLES_SIZE;
    if(IR_buffer_index==MAX30102_BPM_SAMPLES_SIZE-1){
        return true;
    }
    return false;
    
}



/*** READ THE FIFO ***/
bool max30102_i2c_read_multiled_data_burst(struct i2c_device *device)
{
    uint8_t wr_ptr_addr = MAX30102_FIFO_WR_PTR_ADDR;
    uint8_t rd_ptr_addr = MAX30102_FIFO_RD_PTR_ADDR;
    uint8_t wr_ptr, rd_ptr;

    // Read write pointer
    I2C_LOCK_0();
    i2c_master_transmit_receive(device->i2c_dev_handle, &wr_ptr_addr, 1, &wr_ptr, 1, 1000);
    I2C_UNLOCK_0();
    // Read read pointer
    I2C_LOCK_0();
    i2c_master_transmit_receive(device->i2c_dev_handle, &rd_ptr_addr, 1, &rd_ptr, 1, 1000);
    I2C_UNLOCK_0();
    
    int num_samples = (wr_ptr - rd_ptr) & 0x1F; 

    if (num_samples == 0)
        return false;

    int bytes_to_read = num_samples * 6;  // 6 bytes per sample (RED+IR)
    // DBG_PRINTF("bytes to read: %d\n",bytes_to_read);
    // Buffer temporaneo (max 192 bytes per burst → sicuro)
    uint8_t fifo_buffer[192];

    if (bytes_to_read > sizeof(fifo_buffer)) {
        // sicurezza (non dovrebbe accadere)
        bytes_to_read = sizeof(fifo_buffer);
    }

    //trasmit to the fifo address to initialize transmission
    uint8_t fifo_data_addr = MAX30102_FIFO_DATA_ADDR;
    I2C_LOCK_0();
    esp_err_t err = i2c_master_transmit(device->i2c_dev_handle, &fifo_data_addr, 1, 1000);
    I2C_UNLOCK_0();

    if (err != ESP_OK) {
        DBG_PRINTF("Failed to set FIFO_DATA register\n");
        return false;
    }

    //read all the fifo data into a buffer
    I2C_LOCK_0();
    err = i2c_master_receive(device->i2c_dev_handle, fifo_buffer, bytes_to_read, 1000);
    I2C_UNLOCK_0();
    if (err != ESP_OK) {
        DBG_PRINTF("Burst read failed: %d\n", err);
        return false;
    }

    //move the bytes into variables
    for (int i = 0; i < num_samples; i++) {
        int base = i * 6;

        uint32_t red = ((uint32_t)fifo_buffer[base] << 16) |
                       ((uint32_t)fifo_buffer[base+1] << 8) |
                        fifo_buffer[base+2];

        uint32_t ir  = ((uint32_t)fifo_buffer[base+3] << 16) |
                       ((uint32_t)fifo_buffer[base+4] << 8) |
                        fifo_buffer[base+5];

        red &= 0x3FFFF;
        ir  &= 0x3FFFF;

        // Debug
        // DBG_PRINTF("RED_RAW: %lu IR_RAW: %lu\n", red, ir);

        update_red_buffers(red);
        if (update_ir_buffers(ir)) { //used to calculate bpm
            return true; 
        }

        taskYIELD();
    }

    return false;
}
