#ifndef HR_LOGIC_H_
#define HR_LOGIC_H_

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "MAX30102_definitions.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "macros.h"

int16_t     get_RED_AC(uint32_t sample);
int16_t     get_IR_AC(uint32_t sample);
int16_t     get_IR_AC2(int32_t x);
bool        beat_detected(int16_t ir_ac);
void        calculateBPM(int16_t ir_ac_curr,float *BPM,float *AVG_BPM);

#endif /* HR_LOGIC_H_ */