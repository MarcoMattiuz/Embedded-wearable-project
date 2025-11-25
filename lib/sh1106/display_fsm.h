#ifndef __DISPLAY_FSM_H__
#define __DISPLAY_FSM_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h" // for vTaskDelay()
#include "freertos/queue.h"
#include "lcd_sh1106.h"
#include "font5x7.h"
#include "bitmaps.h"
#include "string.h"
#include <driver/gpio.h>
#include "esp_timer.h"

#define PUSH_BUTTON_GPIO GPIO_NUM_23
#define LONG_PRESS_MS 1000 // 1 second
#define DEBOUNCE_MS 50

typedef enum
{
    STATE_BPM,
    STATE_WEATHER,
} State_t;

typedef enum
{
    SUNNY,
    CLOUDY,
    RAINY,
} WeatherType;

typedef enum
{
    EVT_BUTTON_EDGE,
    EVT_LONG_PRESS
} EventType;

typedef struct
{
    State_t state;
    void (*state_function)(esp_lcd_panel_handle_t *);
} StateMachine_t;


extern WeatherType weather;
extern uint8_t buffer_data[SH1106_BUFFER_SIZE];
extern State_t current_state;
extern QueueHandle_t button_queue;
extern TimerHandle_t long_press_timer;
extern bool long_press_triggered;
extern uint32_t last_button_isr_time;
extern StateMachine_t fsm[];
void GPIO_init();


#endif
