#ifndef __INTERRUPT_API_H___
#define __INTERRUPT_API_H___

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define WRIST_DET_INTERRUPT_REG 0x00
BaseType_t xHigherPriorityTaskWoken = pdTRUE;

static void IRAM_ATTR gpio_isr_handler     (void* args);
void                  interrupt_setup_init ();

#endif