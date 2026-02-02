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
#include "global_param.h"
#include "macros.h"



typedef struct
{
    State_t state;
    void (*state_function)(esp_lcd_panel_handle_t *, struct global_param *);
} StateMachine_t;


extern WeatherType weather;
extern uint8_t buffer_data[SH1106_BUFFER_SIZE];
extern State_t current_state;
extern State_t next_state;
extern QueueHandle_t event_queue;
extern TimerHandle_t long_press_timer_handle;
extern TimerHandle_t refresh_timer_handle;
extern TimerHandle_t frame_timer_handle;
extern bool long_press_triggered;
extern uint32_t last_button_isr_time;
extern StateMachine_t fsm[];

void GPIO_init();
extern State_t get_next_state(State_t s);
void fn_BPM(esp_lcd_panel_handle_t *, struct global_param *param);
void fn_WEATHER(esp_lcd_panel_handle_t *, struct global_param *param);
void fn_CLOCK(esp_lcd_panel_handle_t *, struct global_param *param);
void fn_BATTERY(esp_lcd_panel_handle_t *, struct global_param *param);
void fn_STEPS(esp_lcd_panel_handle_t *, struct global_param *param);
void fn_CO2(esp_lcd_panel_handle_t *, struct global_param *param);

void long_press_timer_handler(TimerHandle_t xTimer);
void refresh_timer_handler(TimerHandle_t xTimer);

#endif
