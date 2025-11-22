
#include "freertos/FreeRTOS.h"
#include "freertos/task.h" // for vTaskDelay()

#include "lcd_sh1106.h"
#include "font5x7.h"
#include "bitmaps.h"
#include "string.h"

typedef enum
{
    STATE_INIT,
    STATE_BPM,
    STATE_WEATHER,
    NUM_STATES
} State_t;
typedef enum
{
    SUNNY,
    CLOUDY,
    RAINY,
} WeatherType;

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
State_t current_state = STATE_INIT;
esp_lcd_panel_handle_t panel_handle = NULL;


StateMachine_t fsm[] = {
    {STATE_INIT, fn_INIT},
    {STATE_BPM, fn_BPM},
    {STATE_WEATHER, fn_WEATHER},
    };

void fn_INIT()
{
    panel_handle = lcd_init();
    memset(buffer_data, 0, sizeof(buffer_data)); // fill with 0 → all pixels off
    TurnLcdOn(panel_handle);
    current_state = STATE_BPM;
}
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
        //drawBitmapToBuffer(sunnyBitmap, buffer_data, 0, 0, 64, 64);
        drawStringToBuffer("sdaoihdsaoisdaoi",buffer_data,0,0);
        drawBufferToLcd(buffer_data, panel_handle);
        break;
    default:
        break;
    }

    vTaskDelay(pdMS_TO_TICKS(200));
    current_state = STATE_BPM;
}

void app_main(void)
{
    while (1)
    {
        if (current_state < NUM_STATES)
        {
            
            (*fsm[current_state].state_function)();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else
        {
            /* serious error */
        }
    }
}