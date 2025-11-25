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


struct ppg_task_params {
    struct i2c_device *device;
    i2c_master_bus_handle_t bus;
};

static        i2c_master_bus_handle_t i2c_bus_0;
static        i2c_master_bus_handle_t i2c_bus_1;
static struct ppg_task_params         parameters_ppg_max30102;
static struct i2c_device              max30102_device;
static struct i2c_device              mpu6050_device;

float BPM=0.0f;
float AVG_BPM=0.0f;

void add_device_MAX30102(struct i2c_device* device){
    
    esp_err_t esp_ret;

    //setup device
    device->i2c_dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    device->i2c_dev_config.device_address = I2C_MAX30102_ADDR;
    device->i2c_dev_config.scl_speed_hz = I2C_FREQ_HZ;

    //initialize max30102_dev on the bus
    esp_ret = i2c_master_bus_add_device(i2c_bus_0, &device->i2c_dev_config, &device->i2c_dev_handle);
    if (esp_ret != ESP_OK) {
        DBG_PRINTF("Failed to add MAX30102 device to I2C bus: %d\n", esp_ret);
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
        DBG_PRINTF("Failed to add MPU6050 device to I2C bus: %d\n", esp_ret);
        abort();
    }
}

void PPG_sensor_task(void* parameters){
    //get parameters
    struct ppg_task_params *params = (struct ppg_task_params *) parameters;
    struct i2c_device *device = params->device;
    esp_err_t esp_ret;  
    
    
    esp_ret = init_multiled_mode(device,0x1F, 0x1F, MAX30102_SPO2_RANGE_4096 | MAX30102_SPO2_50_SPS | MAX30102_SPO2_LED_PW_411);
    if (esp_ret != ESP_OK) {
        DBG_PRINTF("Failed to initialize multi-LED mode: %d\n", esp_ret);
        abort();
    }

    // set FIFO configuration: sample averaging = 4 (every sample is avg of 4 misurations), rollover enabled, almost full = 10
    esp_ret = max30102_set_register(device, MAX30102_FIFO_CFG_ADDR, MAX30102_SMP_AVE_2 | MAX30102_FIFO_ROLL_OVER | 0x0A);
    if (esp_ret != ESP_OK) {
        DBG_PRINTF("Failed to configure FIFO: %d\n", esp_ret);
        abort();
    }

    // Reset FIFO pointers
    esp_ret = reset_fifo_registers(device);
    if (esp_ret != ESP_OK) {
        DBG_PRINTF("Failed to reset FIFO registers: %d\n", esp_ret);
        abort();
    }

    
    while (1)
    {   

        if(max30102_i2c_read_multiled_data_burst(device)){
            int MAX = -100000;
            int MIN = 100000;
            for(int i=0;i<MAX30102_BPM_SAMPLES_SIZE;i++){
                if(IR_ac_buffer[i]>MAX){
                    MAX = IR_ac_buffer[i];
                }   
                if(IR_ac_buffer[i]<MIN){
                    MIN = IR_ac_buffer[i];
                }
                // DBG_PRINTF("%d - IR_RAW: %lu - IR_AC: %d\n",i,IR_buffer[i],IR_ac_buffer[i]);
                DBG_PRINTF("%d - IR_RAW: %lu - IR_AC: %d\n",i,IR_buffer[i],IR_ac_buffer[i]);
                calculateBPM(IR_ac_buffer[i],&BPM,&AVG_BPM);
            }
            DBG_PRINTF("BPM: %f,AVG_BPM: %f\n",BPM,AVG_BPM);
            DBG_PRINTF("MAX_AC: %d, MIN_AC: %d\n",MAX,MIN);
        }

        vTaskDelay(250/ portTICK_PERIOD_MS); //keep it 250ms/300ms, it needs to wait for the fifo to be popolated with samples
    }
}

void app_main() {

    // I2C busses init
    init_I2C_bus_PORT0 (&i2c_bus_0);
    init_I2C_bus_PORT1 (&i2c_bus_1);

    add_device_MAX30102(&max30102_device);
    add_device_MPU6050 (&mpu6050_device);

    // parameters init
    parameters_ppg_max30102.bus = i2c_bus_0;
    parameters_ppg_max30102.device = &max30102_device;

    xTaskCreate(
        PPG_sensor_task,
        "PPG_sensor_task_debug",
        4096,
        &parameters_ppg_max30102,   
        5,
        NULL
    );

    // xTaskCreate(
    //     task_acc,
    //     "task_acc_debug",
    //     4096,
    //     &mpu6050_device, 
    //     1,
    //     NULL
    // );
}