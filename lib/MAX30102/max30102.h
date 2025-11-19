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


/*!
 * @brief Set a register value on the MAX30102 sensor
 *
 * @param[in] device : Pointer to the MAX30102 device structure
 * @param[in] reg    : Register address to write to
 * @param[in] mode   : Value to write to the register
 *
 * @return ESP error code
 * @retval ESP_OK : Success
 * @retval Other  : I2C communication error
 */



esp_err_t max30102_set_register(struct max30102_dev *device, uint8_t reg, uint8_t mode);
esp_err_t init_multiled_mode(struct max30102_dev *device, uint8_t led_red_power, uint8_t led_ir_power, uint8_t SPO2_config);
esp_err_t init_hr_mode(struct max30102_dev *device, uint8_t led_red_power, uint8_t led_ir_power, uint8_t SPO2_config);

esp_err_t reset_fifo_registers(struct max30102_dev *device);
void max30102_i2c_read_multiled_data_burst(struct max30102_dev *device);
bool max30102_i2c_read_hr_data_burst(struct max30102_dev *device);
void max30102_i2c_read_multiled_data_one(struct max30102_dev *device);
void max30102_i2c_read_hr_data_one(struct max30102_dev *device);
void max30102_i2c_read_multiled_data_one_buffer(struct max30102_dev *device);
bool beat_detected(int16_t ir_ac);
uint32_t get_time_ms(void);

extern uint32_t RED_buffer[MAX30102_BPM_SAMPLES_SIZE];
extern int16_t RED_ac_buffer[MAX30102_BPM_SAMPLES_SIZE];
extern int RED_buffer_index; //shared index between red buffers
extern uint32_t IR_buffer[MAX30102_BPM_SAMPLES_SIZE];
extern int16_t IR_ac_buffer[MAX30102_BPM_SAMPLES_SIZE];
extern int IR_buffer_index; //shared index between ir buffers
extern int head_beat_count;
extern uint32_t sample_counter; // Global sample counter for accurate BPM calculation



#endif /* MAX30102_H_ */