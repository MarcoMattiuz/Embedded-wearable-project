#include "display_fsm.h"
#include "string.h"

void fn_BPM(esp_lcd_panel_handle_t *, struct global_param *param);
void fn_WEATHER(esp_lcd_panel_handle_t *, struct global_param *param);
static void IRAM_ATTR button_isr(void *);
void long_press_timer_handler(TimerHandle_t xTimer);
void refresh_timer_handler(TimerHandle_t xTimer);

StateMachine_t fsm[] = {
    {STATE_BPM, fn_BPM},
    {STATE_WEATHER, fn_WEATHER}};

WeatherType weather = SUNNY;
uint8_t buffer_data[SH1106_BUFFER_SIZE];
State_t current_state = STATE_BPM;
State_t next_state = STATE_WEATHER;

QueueHandle_t event_queue = NULL;
TimerHandle_t long_press_timer_handle;
TimerHandle_t refresh_timer_handle;
TimerHandle_t frame_timer_handle;

bool long_press_triggered = false;
uint32_t last_button_isr_time = 0;

int test = 0;

void fn_BPM(esp_lcd_panel_handle_t *panel_handle, struct global_param *param)
{
    memset(buffer_data, 0, sizeof(buffer_data));

    if (param->show_heart)
    {
        drawBitmapToBuffer(heartBitmap, buffer_data, 0, 0, 64, 64);
    }

    char str[6];
    // snprintf(str, sizeof(str), "%d", param->AVG_BPM);

    drawStringToBuffer(str, buffer_data, 0, 0);

    drawBufferToLcd(buffer_data, *panel_handle);

    next_state = STATE_WEATHER;
    current_state = STATE_BPM;
}

void fn_WEATHER(esp_lcd_panel_handle_t *panel_handle, struct global_param *param)
{
    memset(buffer_data, 0, sizeof(buffer_data));

    switch (weather)
    {
    case SUNNY:
        drawBitmapToBuffer(sunnyBitmap, buffer_data, 0, 0, 64, 64);
        drawBufferToLcd(buffer_data, *panel_handle);
        break;
    default:
        break;
    }

    next_state = STATE_BPM;
    current_state = STATE_WEATHER;
}

void long_press_timer_handler(TimerHandle_t xTimer)
{
    EventType evt = EVT_LONG_PRESS;
    xQueueSend(event_queue, &evt, 0);
}

void refresh_timer_handler(TimerHandle_t xTimer)
{
    test++;
    EventType evt = EVT_REFRESH;
    xQueueSend(event_queue, &evt, 0);
}

void frame_timer_handler(TimerHandle_t xTimer)
{

    EventType evt = EVT_FRAME;
    xQueueSend(event_queue, &evt, 0);
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
    
    
    
    // (event_queue, &evt, NULL);
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
    event_queue = xQueueCreate(5, sizeof(EventType));

    // Install ISR service and add handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PUSH_BUTTON_GPIO, button_isr, NULL);

    // timer config
    long_press_timer_handle = xTimerCreate(
        "long_press",
        pdMS_TO_TICKS(LONG_PRESS_MS),
        pdFALSE, // one-shot timer
        NULL,
        long_press_timer_handler);

    refresh_timer_handle = xTimerCreate(
        "refresh",
        pdMS_TO_TICKS(REFRESH_TIME_MS),
        pdTRUE, // repeating timer
        NULL,
        refresh_timer_handler);
    frame_timer_handle = xTimerCreate(
        "refresh",
        pdMS_TO_TICKS(FRAME_TIME_MS),
        pdTRUE, // repeating timer
        NULL,
        frame_timer_handler);
}


