#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "MAX30102_definitions.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "max30102.h"
#include "MPU6050_api.h"
#include "I2C_api.h"
#include "reg.h"
#include "esp_err.h"
#include "macros.h"
#include "esp_task_wdt.h"

struct ppg_task_params {
    struct i2c_device *device;
    i2c_master_bus_handle_t bus;
};

static        i2c_master_bus_handle_t i2c_bus_0;
static        i2c_master_bus_handle_t i2c_bus_1;
static struct ppg_task_params         parameters_ppg_max30102;
static struct i2c_device              max30102_device;
static struct i2c_device              mpu6050_device;

void add_device_MAX30102(struct i2c_device* device){
    
    esp_err_t esp_ret;

    //setup device
    device->i2c_dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    device->i2c_dev_config.device_address = I2C_MAX30102_ADDR;
    device->i2c_dev_config.scl_speed_hz = I2C_FREQ_HZ;

    //initialize max30102_dev on the bus
    esp_ret = i2c_master_bus_add_device(i2c_bus_0, &device->i2c_dev_config, &device->i2c_dev_handle);
    if (esp_ret != ESP_OK) {
        printf("Failed to add MAX30102 device to I2C bus: %d\n", esp_ret);
        abort();
    }
}

void add_device_MPU6050(struct i2c_device* device){
    
    esp_err_t esp_ret;

    //setup device
    device->i2c_dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    device->i2c_dev_config.device_address = I2C_MPU6050_ADDR;
    device->i2c_dev_config.scl_speed_hz = I2C_FREQ_HZ;

    //initialize MPU6050 on the bus
    esp_ret = i2c_master_bus_add_device(i2c_bus_1, &device->i2c_dev_config, &device->i2c_dev_handle);
    if (esp_ret != ESP_OK) {
        printf("Failed to add MPU6050 device to I2C bus: %d\n", esp_ret);
        abort();
    }
}

void PPG_sensor_task(void* parameters){
    //get parameters
    struct ppg_task_params *params = (struct ppg_task_params *) parameters;
    struct i2c_device *device = params->device;
    i2c_master_bus_handle_t i2c_bus = params->bus;
    esp_err_t esp_ret;  
    
    // Initialize MAX30102 in hr mode with balanced LED power
    esp_ret = init_hr_mode(device,0x1F, 0x1F, MAX30102_SPO2_RANGE_4096 | MAX30102_SPO2_50_SPS | MAX30102_SPO2_LED_PW_411);
    if (esp_ret != ESP_OK) {
        printf("Failed to initialize multi-LED mode: %d\n", esp_ret);
        abort();
    }

    // set FIFO configuration: sample averaging = 4 (every sample is avg of 4 misurations), rollover enabled, almost full = 10
    esp_ret = max30102_set_register(device, MAX30102_FIFO_CFG_ADDR, MAX30102_SMP_AVE_2 | MAX30102_FIFO_ROLL_OVER | 0x0A);
    if (esp_ret != ESP_OK) {
        printf("Failed to configure FIFO: %d\n", esp_ret);
        abort();
    }

    // Reset FIFO pointers
    esp_ret = reset_fifo_registers(device);
    if (esp_ret != ESP_OK) {
        printf("Failed to reset FIFO registers: %d\n", esp_ret);
        abort();
    }

    float BPM=0.0f;
    float AVG_BPM=0.0f;
    while (1)
    {   
        // max30102_i2c_read_multiled_data_one(device);
        // for(int i=0;i<MAX30102_BPM_SAMPLES_SIZE;i++){
        //     max30102_i2c_read_multiled_data_one_buffer(device);
        // }
        // vTaskDelay(100 / portTICK_PERIOD_MS); 
        // for(int i=0;i<MAX30102_BPM_SAMPLES_SIZE;i++){
        //     printf("%d - IR_RAW: %lu - IR_AC: %d\n",i,IR_buffer[i],IR_ac_buffer[i]);
        // }
       
        if(max30102_i2c_read_hr_data_burst(device)){
            for(int i=0;i<MAX30102_BPM_SAMPLES_SIZE;i++){
                printf("%d - IR_RAW: %lu - IR_AC: %d\n",i,IR_buffer[i],IR_ac_buffer[i]);
                calculateBPM(IR_ac_buffer[i],&BPM,&AVG_BPM);
            }
            printf("BPM: %f,AVG_BPM: %f\n",BPM,AVG_BPM);
            vTaskDelay(100/ portTICK_PERIOD_MS);
        }

        // vTaskDelay(100/ portTICK_PERIOD_MS);
    }
}

void app_main() {

    esp_task_wdt_deinit();

    // Inizializza I2C prima di usarlo
    init_I2C_bus_PORT0(&i2c_bus_0);
    init_I2C_bus_PORT1(&i2c_bus_1);

    add_device_MAX30102(&max30102_device);
    add_device_MPU6050 (&mpu6050_device);

    // Inizializza i parametri per il task
    parameters_ppg_max30102.bus = i2c_bus_0;
    parameters_ppg_max30102.device = &max30102_device;

    xTaskCreate(
        PPG_sensor_task,
        "PPG_sensor_task_debug",
        4096,
        &parameters_ppg_max30102,   
        1,
        NULL
    );

    xTaskCreate(
        task_acc,
        "task_acc_debug",
        4096,
        &mpu6050_device, 
        2,
        NULL
    );
}