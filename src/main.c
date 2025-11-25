
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
    void (*state_function)(void);
} StateMachine_t;

void fn_INIT(void);
void fn_BPM(void);
void fn_WEATHER(void);

WeatherType weather = SUNNY;
uint8_t buffer_data[SH1106_BUFFER_SIZE];
State_t current_state = STATE_BPM;
esp_lcd_panel_handle_t panel_handle = NULL;
QueueHandle_t button_queue = NULL;
TimerHandle_t long_press_timer;
bool long_press_triggered = false;
static uint32_t last_button_isr_time = 0;

StateMachine_t fsm[] = {
    {STATE_BPM, fn_BPM},
    {STATE_WEATHER, fn_WEATHER}};

void fn_BPM()
{
    memset(buffer_data, 0, sizeof(buffer_data));
    drawBitmapToBuffer(heartBitmap, buffer_data, 0, 0, 64, 64);
    drawBufferToLcd(buffer_data, panel_handle);

    vTaskDelay(pdMS_TO_TICKS(200));
    current_state = STATE_WEATHER;
}
void fn_WEATHER()
{
    memset(buffer_data, 0, sizeof(buffer_data));

    switch (weather)
    {
    case SUNNY:
        drawBitmapToBuffer(sunnyBitmap, buffer_data, 0, 0, 64, 64);
        drawBufferToLcd(buffer_data, panel_handle);
        break;
    default:
        break;
    }

    vTaskDelay(pdMS_TO_TICKS(200));
    current_state = STATE_BPM;
}

static void IRAM_ATTR button_isr(void *arg)
{
    uint32_t now = xTaskGetTickCountFromISR();

    // Debounce: ignore if signal comes too soon
    if ((now - last_button_isr_time) < pdMS_TO_TICKS(DEBOUNCE_MS))
    {
        return;
    }
    last_button_isr_time = now;

    EventType evt = EVT_BUTTON_EDGE;
    xQueueSendFromISR(button_queue, &evt, NULL);
}

void long_press_timer_handler(TimerHandle_t xTimer)
{
    EventType evt = EVT_LONG_PRESS;
    xQueueSend(button_queue, &evt, 0);
}

void GPIO_init()
{

    // push button GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << PUSH_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE};
    gpio_config(&io_conf);

    // button queue to wake up main task
    button_queue = xQueueCreate(10, sizeof(EventType));

    // Install ISR service and add handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PUSH_BUTTON_GPIO, button_isr, NULL);

    // timer config
    long_press_timer = xTimerCreate(
        "long_press",
        pdMS_TO_TICKS(LONG_PRESS_MS),
        pdFALSE, // one-shot timer
        NULL,
        long_press_timer_handler);
}

void app_main(void)
{

    GPIO_init();

    panel_handle = lcd_init();
    memset(buffer_data, 0, sizeof(buffer_data)); // fill with 0 → all pixels off

    bool LCD_ON = false;
    EventType evt;
    while (1)
    {
        // isr wakes up main on button pressed/released
        if (xQueueReceive(button_queue, &evt, portMAX_DELAY))
        {

            if (evt == EVT_BUTTON_EDGE){
                int level = gpio_get_level(PUSH_BUTTON_GPIO);

                if (level == 0) // button pressed -> start long press detection timer
                {
                    long_press_triggered = false;
                    xTimerStart(long_press_timer, 0);
                }
                else // released
                {
                    xTimerStop(long_press_timer, 0);

                    if (!long_press_triggered && LCD_ON) // short press
                    {
                        (*fsm[current_state].state_function)();
                    }
                }
            }
            else if (evt == EVT_LONG_PRESS)
            {
                long_press_triggered = true;

                LCD_ON = !LCD_ON;
                if (LCD_ON)
                {
                    TurnLcdOn(panel_handle);
                    current_state = STATE_BPM;
                    (*fsm[current_state].state_function)();
                }
                else
                {
                    TurnLcdOff(panel_handle);
                }
            }
        }
    }
}
