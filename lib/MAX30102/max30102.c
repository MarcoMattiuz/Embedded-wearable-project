#include "MAX30102.h"

uint32_t RED_buffer[MAX30102_BPM_SAMPLES_SIZE] = {0};
int RED_buffer_index = 0;
uint32_t IR_buffer[MAX30102_BPM_SAMPLES_SIZE] = {0};
int IR_buffer_index = 0;
int16_t RED_ac_buffer[MAX30102_BPM_SAMPLES_SIZE] = {0};
int16_t IR_ac_buffer[MAX30102_BPM_SAMPLES_SIZE] = {0};
int head_beat_count = 0;




esp_err_t max30102_set_register(struct max30102_dev *device, uint8_t reg,uint8_t mode){
    uint8_t txbuf[2];
    txbuf[0] = reg;
    txbuf[1] = mode;
    esp_err_t esp_ret = i2c_master_transmit(device->i2c_dev_handle, txbuf, sizeof(txbuf), 1000);
    return esp_ret;
}

esp_err_t init_multiled_mode(struct max30102_dev *device, uint8_t led_red_power, uint8_t led_ir_power, uint8_t SPO2_config) {
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


esp_err_t init_hr_mode(struct max30102_dev *device, uint8_t led_red_power, uint8_t led_ir_power, uint8_t SPO2_config) {
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




esp_err_t reset_fifo_registers(struct max30102_dev *device) {
    esp_err_t esp_ret;
    esp_ret = max30102_set_register(device, MAX30102_FIFO_WR_PTR_ADDR, 0x00);
    if (esp_ret != ESP_OK) {
        printf("Failed to reset FIFO write pointer: %d\n", esp_ret);
        abort();
    }
    
    esp_ret = max30102_set_register(device, MAX30102_FIFO_RD_PTR_ADDR, 0x00);
    if (esp_ret != ESP_OK) {
        printf("Failed to reset FIFO read pointer: %d\n", esp_ret);
        abort();
    }
    
    esp_ret = max30102_set_register(device, MAX30102_FIFO_OVF_CTR_ADDR, 0x00);
    if (esp_ret != ESP_OK) {
        printf("Failed to reset FIFO overflow counter: %d\n", esp_ret);
        abort();
    }
    printf("FIFO pointers reset\n");
    return ESP_OK;
}

static void update_red_buffers(uint32_t value) {
    RED_buffer[RED_buffer_index] = value;
    RED_ac_buffer[RED_buffer_index] = get_RED_AC(value);
    RED_buffer_index = (RED_buffer_index + 1) % MAX30102_BPM_SAMPLES_SIZE;
}
static bool update_ir_buffers(uint32_t value) {
    IR_buffer[IR_buffer_index] = value;
    IR_ac_buffer[IR_buffer_index] = get_IR_AC2(value); //TODO: check
    IR_buffer_index = (IR_buffer_index + 1) % MAX30102_BPM_SAMPLES_SIZE;
    if(IR_buffer_index==MAX30102_BPM_SAMPLES_SIZE-1){
        return true;
    }
    return false;
    
}


void max30102_i2c_read_hr_data_one(struct max30102_dev *device) {

    uint8_t fifo_data_addr = MAX30102_FIFO_DATA_ADDR;
    uint8_t sample_data[3]; // 3 bytes per LED1 

    // Read 6 bytes from the FIFO_DATA register
    esp_err_t read_result = i2c_master_transmit_receive(device->i2c_dev_handle,
                                                        &fifo_data_addr, 1,
                                                        sample_data, 3, 1000);
    
    if (read_result == ESP_OK) {
        // Increment sample counter for this sample
        
        
        // Reconstruct 18-bit values for each LED
        uint32_t led1_value = ((uint32_t)sample_data[0] << 16) | /* LED1 is RED */
                                ((uint32_t)sample_data[1] << 8) | 
                                sample_data[2];
        led1_value &= 0x3FFFF; // Mask for 18 bits
        

        
       
        printf("RED: %lu, ", led1_value);
        if(led1_value < 10000) {
            // Skip invalid readings
            printf(" --- Invalid IR reading --- \n");
        }else{
            // int red_ac_curr = get_RED_AC(led1_value);
            int ir_ac_curr = get_IR_AC2(led1_value);
            printf("IR_AC: %d\n",ir_ac_curr);
        }
        
        
    } else {
        printf("Failed to read FIFO data: %d\n", read_result);
        abort();
    }
}

bool max30102_i2c_read_hr_data_burst(struct max30102_dev *device) {
    
    uint8_t wr_ptr_addr = MAX30102_FIFO_WR_PTR_ADDR;
    uint8_t rd_ptr_addr = MAX30102_FIFO_RD_PTR_ADDR;
    uint8_t wr_ptr, rd_ptr;

    i2c_master_transmit_receive(device->i2c_dev_handle, &wr_ptr_addr, 1, &wr_ptr, 1, 1000);
    i2c_master_transmit_receive(device->i2c_dev_handle, &rd_ptr_addr, 1, &rd_ptr, 1, 1000);

    uint8_t num_samples = (wr_ptr - rd_ptr) & 0x1F;  // FIFO è profonda 32 campioni (5 bit)
    
    // printf("Running... WR_PTR: %d, RD_PTR: %d, Samples: %d\n", wr_ptr, rd_ptr, num_samples);
    
    if (num_samples > 0) {
        
        // printf("Reading %d samples from FIFO...\n", num_samples);
        for (int i = 0; i < num_samples; i++) {
            // In modalità MULTILED con 2 LED, ogni campione è di 6 bytes (3 per LED)
            uint8_t fifo_data_addr = MAX30102_FIFO_DATA_ADDR;
            uint8_t sample_data[3]; // 3 bytes per LED1 + 3 bytes per LED2
            
            // Leggi 6 bytes dal registro FIFO_DATA
            esp_err_t read_result = i2c_master_transmit_receive(device->i2c_dev_handle, 
                                                                &fifo_data_addr, 1, 
                                                                sample_data, 3, 1000);
            
            if (read_result == ESP_OK) {
                
                // Ricostruisci i valori a 18 bit per ciascun LED
                uint32_t led1_value = ((uint32_t)sample_data[0] << 16) | 
                                        ((uint32_t)sample_data[1] << 8) | 
                                        sample_data[2];
                led1_value &= 0x3FFFF; // Maschera per 18 bit
                if(led1_value >= 10000){
                    if(update_ir_buffers(led1_value)){
                    return true; //buffer pieno
                    }
                }else{
                    printf("--not reading properly--\n");
                }
                
                
            } else {
                printf("Failed to read FIFO data: %d\n", read_result);
                break;
            }
        }
    
    }

    return false;
}




float BPM=0.0f,AVG_BPM=0.0f;
void max30102_i2c_read_multiled_data_one(struct max30102_dev *device) {

    // In multiled mode with 2 LEDs, each sample is 6 bytes (3 per LED)
    uint8_t fifo_data_addr = MAX30102_FIFO_DATA_ADDR;
    uint8_t sample_data[6]; // 3 bytes per LED1 + 3 bytes per LED2

    // Read 6 bytes from the FIFO_DATA register
    esp_err_t read_result = i2c_master_transmit_receive(device->i2c_dev_handle,
                                                        &fifo_data_addr, 1,
                                                        sample_data, 6, 1000);
    
    if (read_result == ESP_OK) {
        // Increment sample counter for this sample
        sample_counter++;
        
        // Reconstruct 18-bit values for each LED
        uint32_t led1_value = ((uint32_t)sample_data[0] << 16) | /* LED1 is RED */
                                ((uint32_t)sample_data[1] << 8) | 
                                sample_data[2];
        led1_value &= 0x3FFFF; // Mask for 18 bits
        uint32_t led2_value = ((uint32_t)sample_data[3] << 16) |  /* LED2 is IR */
                                ((uint32_t)sample_data[4] << 8) | 
                                sample_data[5];
        led2_value &= 0x3FFFF; // Mask for 18 bits

        
       
        printf("RED: %lu, IR: %lu, ", led1_value, led2_value);
        if(led2_value < 10000) {
            // Skip invalid readings
            printf(" --- Invalid IR reading --- \n");
        }else{
            int red_ac_curr = get_RED_AC(led1_value);
            int ir_ac_curr = get_IR_AC(led2_value);
            if(ir_ac_curr < 0){
                printf("IR_AC: %d       ↓↓↓\n", ir_ac_curr);
            }else{
                printf("IR_AC: %d       ↑↑↑\n", ir_ac_curr);
            }
            
            calculateBPM(ir_ac_curr,&BPM,&AVG_BPM);

            printf(" BPM: %.1f", BPM);
            printf(" AVG BPM: %.1f", AVG_BPM);
            printf("\n");
        }
        
        
    } else {
        printf("Failed to read FIFO data: %d\n", read_result);
        abort();
    }
}

void max30102_i2c_read_multiled_data_one_buffer(struct max30102_dev *device) {

    // In multiled mode with 2 LEDs, each sample is 6 bytes (3 per LED)
    uint8_t fifo_data_addr = MAX30102_FIFO_DATA_ADDR;
    uint8_t sample_data[6]; // 3 bytes per LED1 + 3 bytes per LED2

    // Read 6 bytes from the FIFO_DATA register
    esp_err_t read_result = i2c_master_transmit_receive(device->i2c_dev_handle,
                                                        &fifo_data_addr, 1,
                                                        sample_data, 6, 1000);
    
    if (read_result == ESP_OK) {
        // Increment sample counter for this sample
        
        
        // Reconstruct 18-bit values for each LED
        uint32_t led1_value = ((uint32_t)sample_data[0] << 16) | /* LED1 is RED */
                                ((uint32_t)sample_data[1] << 8) | 
                                sample_data[2];
        led1_value &= 0x3FFFF; // Mask for 18 bits
        uint32_t led2_value = ((uint32_t)sample_data[3] << 16) |  /* LED2 is IR */
                                ((uint32_t)sample_data[4] << 8) | 
                                sample_data[5];
        led2_value &= 0x3FFFF; // Mask for 18 bits

        
       
        // printf("RED: %lu, IR: %lu, ", led1_value, led2_value);
        // if(led2_value < 10000) {
        //     // Skip invalid readings
        //     printf(" --- Invalid IR reading --- \n");
        // }else{
            // printf("LED2_IR: %lu\n",led2_value);
            update_red_buffers(led1_value);
            update_ir_buffers(led2_value);
        // }
        
        
    } else {
        printf("Failed to read FIFO data: %d\n", read_result);
        abort();
    }
}
    
void max30102_i2c_read_multiled_data_burst(struct max30102_dev *device) {
    
    uint8_t wr_ptr_addr = MAX30102_FIFO_WR_PTR_ADDR;
    uint8_t rd_ptr_addr = MAX30102_FIFO_RD_PTR_ADDR;
    uint8_t wr_ptr, rd_ptr;

    i2c_master_transmit_receive(device->i2c_dev_handle, &wr_ptr_addr, 1, &wr_ptr, 1, 1000);
    i2c_master_transmit_receive(device->i2c_dev_handle, &rd_ptr_addr, 1, &rd_ptr, 1, 1000);

    uint8_t num_samples = (wr_ptr - rd_ptr) & 0x1F;  // FIFO è profonda 32 campioni (5 bit)
    
    // printf("Running... WR_PTR: %d, RD_PTR: %d, Samples: %d\n", wr_ptr, rd_ptr, num_samples);
    
    if (num_samples > 0) {
        
        // printf("Reading %d samples from FIFO...\n", num_samples);
        for (int i = 0; i < num_samples; i++) {
            // In modalità MULTILED con 2 LED, ogni campione è di 6 bytes (3 per LED)
            uint8_t fifo_data_addr = MAX30102_FIFO_DATA_ADDR;
            uint8_t sample_data[6]; // 3 bytes per LED1 + 3 bytes per LED2
            
            // Leggi 6 bytes dal registro FIFO_DATA
            esp_err_t read_result = i2c_master_transmit_receive(device->i2c_dev_handle, 
                                                                &fifo_data_addr, 1, 
                                                                sample_data, 6, 1000);
            
            if (read_result == ESP_OK) {
                // Increment sample counter for each sample read
                sample_counter++;
                
                // Ricostruisci i valori a 18 bit per ciascun LED
                uint32_t led1_value = ((uint32_t)sample_data[0] << 16) | 
                                        ((uint32_t)sample_data[1] << 8) | 
                                        sample_data[2];
                led1_value &= 0x3FFFF; // Maschera per 18 bit
                update_red_buffers(led1_value);
                uint32_t led2_value = ((uint32_t)sample_data[3] << 16) | 
                                        ((uint32_t)sample_data[4] << 8) | 
                                        sample_data[5];
                led2_value &= 0x3FFFF; // Maschera per 18 bit
                update_ir_buffers(led2_value);
            } else {
                printf("Failed to read FIFO data: %d\n", read_result);
                break;
            }
        }
    
    }

    //TODO: check this
    esp_err_t esp_ret = reset_fifo_registers(device);
    if (esp_ret != ESP_OK) {        
        printf("Failed to reset FIFO registers after burst read: %d\n", esp_ret);
        abort();
    }
    
}