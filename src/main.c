<<<<<<< HEAD
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "MAX30102_definitions.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "max30102.h"
#include "MPU6050_api.h"
#include "I2C_api.h"
#include "reg.h"
#include "esp_err.h"
#include "macros.h"
=======

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
>>>>>>> 96dc577a5cd4103de5a0ea23f1154b02468df0bf

typedef enum
{
    STATE_BPM,
    STATE_WEATHER,
} State_t;

<<<<<<< HEAD
struct ppg_task_params {
    struct i2c_device *device;
    i2c_master_bus_handle_t bus;
};

static        i2c_master_bus_handle_t i2c_bus_0;
static        i2c_master_bus_handle_t i2c_bus_1;
static struct ppg_task_params         parameters_ppg_max30102;
static struct i2c_device              max30102_device;
static struct i2c_device              mpu6050_device;

float BPM=0.0f;
float AVG_BPM=0.0f;

void add_device_MAX30102(struct i2c_device* device){
    
    esp_err_t esp_ret;

    //setup device
    device->i2c_dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    device->i2c_dev_config.device_address = I2C_MAX30102_ADDR;
    device->i2c_dev_config.scl_speed_hz = I2C_FREQ_HZ;

    //initialize max30102_dev on the bus
    esp_ret = i2c_master_bus_add_device(i2c_bus_0, &device->i2c_dev_config, &device->i2c_dev_handle);
    if (esp_ret != ESP_OK) {
        DBG_PRINTF("Failed to add MAX30102 device to I2C bus: %d\n", esp_ret);
        abort();
=======
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
>>>>>>> 96dc577a5cd4103de5a0ea23f1154b02468df0bf
    }

    vTaskDelay(pdMS_TO_TICKS(200));
    current_state = STATE_BPM;
}

<<<<<<< HEAD
void add_device_MPU6050(struct i2c_device* device){
    
    esp_err_t esp_ret;

    //setup device
    device->i2c_dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    device->i2c_dev_config.device_address = I2C_MPU6050_ADDR;
    device->i2c_dev_config.scl_speed_hz = I2C_FREQ_HZ;

    //initialize MPU6050 on the bus
    esp_ret = i2c_master_bus_add_device(i2c_bus_1, &device->i2c_dev_config, &device->i2c_dev_handle);
    if (esp_ret != ESP_OK) {
        DBG_PRINTF("Failed to add MPU6050 device to I2C bus: %d\n", esp_ret);
        abort();
=======
static void IRAM_ATTR button_isr(void *arg)
{
    uint32_t now = xTaskGetTickCountFromISR();

    // Debounce: ignore if signal comes too soon
    if ((now - last_button_isr_time) < pdMS_TO_TICKS(DEBOUNCE_MS))
    {
        return;
>>>>>>> 96dc577a5cd4103de5a0ea23f1154b02468df0bf
    }
    last_button_isr_time = now;

    EventType evt = EVT_BUTTON_EDGE;
    xQueueSendFromISR(button_queue, &evt, NULL);
}

<<<<<<< HEAD
void PPG_sensor_task(void* parameters){
    //get parameters
    struct ppg_task_params *params = (struct ppg_task_params *) parameters;
    struct i2c_device *device = params->device;
    esp_err_t esp_ret;  
    
    
    esp_ret = init_multiled_mode(device,0x1F, 0x1F, MAX30102_SPO2_RANGE_4096 | MAX30102_SPO2_50_SPS | MAX30102_SPO2_LED_PW_411);
    if (esp_ret != ESP_OK) {
        DBG_PRINTF("Failed to initialize multi-LED mode: %d\n", esp_ret);
        abort();
    }

    // set FIFO configuration: sample averaging = 4 (every sample is avg of 4 misurations), rollover enabled, almost full = 10
    esp_ret = max30102_set_register(device, MAX30102_FIFO_CFG_ADDR, MAX30102_SMP_AVE_2 | MAX30102_FIFO_ROLL_OVER | 0x0A);
    if (esp_ret != ESP_OK) {
        DBG_PRINTF("Failed to configure FIFO: %d\n", esp_ret);
        abort();
    }

    // Reset FIFO pointers
    esp_ret = reset_fifo_registers(device);
    if (esp_ret != ESP_OK) {
        DBG_PRINTF("Failed to reset FIFO registers: %d\n", esp_ret);
        abort();
    }

    
    while (1)
    {   

        if(max30102_i2c_read_multiled_data_burst(device)){
            int MAX = -100000;
            int MIN = 100000;
            for(int i=0;i<MAX30102_BPM_SAMPLES_SIZE;i++){
                if(IR_ac_buffer[i]>MAX){
                    MAX = IR_ac_buffer[i];
                }   
                if(IR_ac_buffer[i]<MIN){
                    MIN = IR_ac_buffer[i];
                }
                // DBG_PRINTF("%d - IR_RAW: %lu - IR_AC: %d\n",i,IR_buffer[i],IR_ac_buffer[i]);
                DBG_PRINTF("%d - IR_RAW: %lu - IR_AC: %d\n",i,IR_buffer[i],IR_ac_buffer[i]);
                calculateBPM(IR_ac_buffer[i],&BPM,&AVG_BPM);
            }
            DBG_PRINTF("BPM: %f,AVG_BPM: %f\n",BPM,AVG_BPM);
            DBG_PRINTF("MAX_AC: %d, MIN_AC: %d\n",MAX,MIN);
        }

        vTaskDelay(250/ portTICK_PERIOD_MS); //keep it 250ms/300ms, it needs to wait for the fifo to be popolated with samples
    }
}

void app_main() {

    // I2C busses init
    init_I2C_bus_PORT0 (&i2c_bus_0);
    init_I2C_bus_PORT1 (&i2c_bus_1);

    add_device_MAX30102(&max30102_device);
    add_device_MPU6050 (&mpu6050_device);

    // parameters init
    parameters_ppg_max30102.bus = i2c_bus_0;
    parameters_ppg_max30102.device = &max30102_device;

    xTaskCreate(
        PPG_sensor_task,
        "PPG_sensor_task_debug",
        4096,
        &parameters_ppg_max30102,   
        5,
        NULL
    );

    // xTaskCreate(
    //     task_acc,
    //     "task_acc_debug",
    //     4096,
    //     &mpu6050_device, 
    //     1,
    //     NULL
    // );
}
=======
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
>>>>>>> 96dc577a5cd4103de5a0ea23f1154b02468df0bf
