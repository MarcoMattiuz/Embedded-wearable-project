#ifndef MAX30102_H_
#define MAX30102_H_

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "MAX30102_definitions.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hr_logic.h"
#include "I2C_api.h"
#include "macros.h"


esp_err_t   max30102_set_register(struct i2c_device *device, uint8_t reg, uint8_t mode);
esp_err_t   init_multiled_mode(struct i2c_device *device, uint8_t led_red_power, uint8_t led_ir_power, uint8_t SPO2_config);
esp_err_t   init_hr_mode(struct i2c_device *device, uint8_t led_red_power, uint8_t led_ir_power, uint8_t SPO2_config);
esp_err_t   reset_fifo_registers(struct i2c_device *device);
bool        max30102_i2c_read_multiled_data_burst(struct i2c_device *device);
bool        max30102_i2c_read_hr_data_burst(struct i2c_device *device);


extern uint32_t     RED_buffer[MAX30102_BPM_SAMPLES_SIZE];
extern int16_t      RED_ac_buffer[MAX30102_BPM_SAMPLES_SIZE];
extern uint32_t     IR_buffer[MAX30102_BPM_SAMPLES_SIZE];
extern int16_t      IR_ac_buffer[MAX30102_BPM_SAMPLES_SIZE];
extern int          RED_buffer_index; //shared index between red buffers
extern int          IR_buffer_index; //shared index between ir buffers
extern int          head_beat_count;



#endif /* MAX30102_H_ */