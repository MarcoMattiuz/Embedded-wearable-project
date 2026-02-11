#include "display_fsm.h"
#include "string.h"
#include "ble_manager.h"
StateMachine_t fsm[] = {
    {STATE_BPM, fn_BPM},
    {STATE_CLOCK, fn_CLOCK},
    {STATE_WEATHER, fn_WEATHER},
    {STATE_STEPS, fn_STEPS},
    {STATE_BATTERY, fn_BATTERY},
    {STATE_CO2, fn_CO2},
    {STATE_PARTICULATE, fn_PARTICULATE},
};

uint8_t buffer_data[SH1106_BUFFER_SIZE];
State_t current_state = STATE_BPM;
State_t next_state = STATE_CLOCK;

QueueHandle_t event_queue = NULL;
TimerHandle_t long_press_timer_handle;
TimerHandle_t refresh_timer_handle;
TimerHandle_t frame_timer_handle;

bool long_press_triggered = false;
uint32_t last_button_isr_time = 0;

int test = 0;

void show_loading_screen(esp_lcd_panel_handle_t *panel_handle)
{
    for (int i = 0; i < 5; i++)
    {
        memset(buffer_data, 0, sizeof(buffer_data));

        
        drawStringToBuffer("LOADING", buffer_data, 40, 18);

        // Draw circles 
        for (int j = 0; j <= i; j++)
        {
            drawBitmapToBuffer(
                LoadingBitmap,
                buffer_data,
                4 + j * (16+8),
                32,
                16,
                16);
        }

        drawBufferToLcd(buffer_data, *panel_handle);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
void fn_CLOCK(esp_lcd_panel_handle_t *panel_handle, struct global_param *param)
{
    memset(buffer_data, 0, sizeof(buffer_data));


    drawStringToBuffer(param->date, buffer_data, 64 + 5, 28);
    drawStringToBuffer(param->time_str, buffer_data, 64 + 5, 28 + 8);

    if(ble_manager_is_connected()){
        drawBitmapToBuffer(bluetoothBitmap, buffer_data, 112, 0,16,16);
    }
    drawBitmapToBuffer(clockBitmap, buffer_data, 0, 0, 64, 64);
    
    drawBufferToLcd(buffer_data, *panel_handle);
}
void fn_BPM(esp_lcd_panel_handle_t *panel_handle, struct global_param *param)
{
    memset(buffer_data, 0, sizeof(buffer_data));

    if (param->show_heart)
    {
        drawBitmapToBuffer(heartBitmap, buffer_data, 0, 0, 64, 64);
    }

    char str[12];
    snprintf(str, sizeof(str), "%d BPM", param->AVG_BPM);

    drawStringToBuffer(str, buffer_data, 64 + 5, 28);

    drawBufferToLcd(buffer_data, *panel_handle);
}

void fn_STEPS(esp_lcd_panel_handle_t *panel_handle, struct global_param *param)
{
    memset(buffer_data, 0, sizeof(buffer_data));

    char buff[10];
    sprintf(buff, "%d", param->step_cntr);

    drawStringToBuffer(buff, buffer_data, 64 + 5, 28);

    drawBitmapToBuffer(stepsBitmap, buffer_data, 0, 0, 64, 64);

    drawBufferToLcd(buffer_data, *panel_handle);
}


void fn_WEATHER(esp_lcd_panel_handle_t *panel_handle, struct global_param *param)
{
    memset(buffer_data, 0, sizeof(buffer_data));
    char buff[10];
    sprintf(buff, "%.1f C", param->temperature);

    drawStringToBuffer(buff, buffer_data, 64 + 5, 28);
    switch (param->weather)
    {
    case SUNNY:
    {
        drawBitmapToBuffer(sunnyBitmap, buffer_data, 0, 0, 64, 64);
        break;
    }
    case CLOUDY:
    {
        drawBitmapToBuffer(cloudyBitmap, buffer_data, 0, 0, 64, 64);
        break;
    }
    case FOGGY:
    {
        drawBitmapToBuffer(fogBitmap, buffer_data, 0, 0, 64, 64);
        break;
    }
    case SNOWY:
    {
        drawBitmapToBuffer(snowBitmap, buffer_data, 0, 0, 64, 64);
        break;
    }
    case THUNDERSTORM:
    {
        drawBitmapToBuffer(thunderBitmap, buffer_data, 0, 0, 64, 64);
        break;
    }
    case RAINY:
    {
        drawBitmapToBuffer(rainyBitmap, buffer_data, 0, 0, 64, 64);
        break;
    }
    default:
    {
        drawBitmapToBuffer(cloudyBitmap, buffer_data, 0, 0, 64, 64);

        break;
    }
    }
    if (ble_manager_is_connected())
    {
        drawBitmapToBuffer(bluetoothBitmap, buffer_data, 112, 0, 16, 16);
    }

    drawBufferToLcd(buffer_data, *panel_handle);
}

void fn_BATTERY(esp_lcd_panel_handle_t *panel_handle, struct global_param *param)
{
    memset(buffer_data, 0, sizeof(buffer_data));

    switch (param->battery_state)
    {
    case BATTERY_EMPTY:
    {
        drawBitmapToBuffer(emptyBatteryBitmap, buffer_data, 0, 0, 64, 64);
        break;
    }
    case BATTERY_LOW:
    {
        drawBitmapToBuffer(lowBatteryBitmap, buffer_data, 0, 0, 64, 64);
        break;
    }
    case BATTERY_MEDIUM:
    {
        drawBitmapToBuffer(mediumBatteryBitmap, buffer_data, 0, 0, 64, 64);
        break;
    }
    case BATTERY_HIGH:
    {
        drawBitmapToBuffer(highBatteryBitmap, buffer_data, 0, 0, 64, 64);
        break;
    }
    default:
    {
        drawBitmapToBuffer(fullBatteryBitmap, buffer_data, 0, 0, 64, 64);
        break;
    }
    }

    char buff[15];
    sprintf(buff, "%.2f V", param->battery_voltage);

    drawStringToBuffer(buff, buffer_data, 64 + 5, 28);

    drawBufferToLcd(buffer_data, *panel_handle);
}

void fn_CO2(esp_lcd_panel_handle_t *panel_handle, struct global_param *param)
{
    memset(buffer_data, 0, sizeof(buffer_data));

    char level[64];
    char buff[128];
    
    switch (param->CO2_risk_level)
    {   
    case 1:
        sprintf(level, "Excellent");
        break;
    case 2:
        sprintf(level, "Good");
        break;
    case 3:
        sprintf(level, "Fair");
        break;
    case 4:
        sprintf(level, "Poor");
        break;
    case 5:
        sprintf(level, "Bad");
        break;
    default:
        sprintf(level, "Unknown");
        break;
    }
    sprintf(buff, "%d ppm\n%s", param->CO2,level);

    drawStringToBuffer(buff, buffer_data, 64 + 5, 28);

    drawBitmapToBuffer(co2Bitmap, buffer_data, 0, 0, 64, 64);

    if (param->CO2_init_percentage != 100){
        drawBitmapToBuffer(circleArrowBitmap, buffer_data, 112, 0, 16, 16);
        sprintf(buff, "%d%%", param->CO2_init_percentage);
        drawStringToBuffer(buff, buffer_data, 112-20, 3);
    }

        drawBufferToLcd(buffer_data, *panel_handle);

}

void fn_PARTICULATE(esp_lcd_panel_handle_t *panel_handle, struct global_param *param)
{
    memset(buffer_data, 0, sizeof(buffer_data));

    char buff[36];

    sprintf(buff, "%d ppb", param->particulate);

    drawStringToBuffer(buff, buffer_data, 64 + 5, 28);

    drawBitmapToBuffer(particulateBitmap, buffer_data, 0, 0, 64, 64);

    if (param->CO2 == 0)
    {
        drawBitmapToBuffer(circleArrowBitmap, buffer_data, 112, 0, 16, 16);
    }
    drawBufferToLcd(buffer_data, *panel_handle);
}

void long_press_timer_handler(TimerHandle_t xTimer)
{
    EventType evt = EVT_LONG_PRESS;
    xQueueSend(event_queue, &evt, 0);
}

void refresh_timer_handler(TimerHandle_t xTimer)
{
    EventType evt = EVT_REFRESH;
    xQueueSend(event_queue, &evt, 0);
}

void frame_timer_handler(TimerHandle_t xTimer)
{
    EventType evt = EVT_REFRESH;
    global_parameters.show_heart = !global_parameters.show_heart;
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

    xQueueSendFromISR(event_queue, &evt, NULL);
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

    // on screen data refresh timer
    refresh_timer_handle = xTimerCreate(
        "refresh",
        pdMS_TO_TICKS(REFRESH_TIME_MS),
        pdTRUE, // repeating timer
        NULL,
        refresh_timer_handler);
    // bpm screen blinking heart timer
    frame_timer_handle = xTimerCreate(
        "frame refresh",
        pdMS_TO_TICKS(FRAME_TIME_MS),
        pdTRUE, // repeating timer
        NULL,
        frame_timer_handler);
}

State_t get_next_state(State_t s)
{
    return (State_t)((s + 1) % STATE_COUNT);
}