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
#include "display_fsm.h"
#include "global_param.h"
#include "esp_log.h"
#include "ble_manager.h"
#include "driver/touch_pad.h"
#include <sys/time.h>
#include <time.h>
#include "esp_task_wdt.h"

struct ppg_task_params
{
    struct i2c_device *device;
    i2c_master_bus_handle_t bus;
};

BaseType_t retF;

static struct global_param global_parameters;
static i2c_master_bus_handle_t i2c_bus_0;
static i2c_master_bus_handle_t i2c_bus_1;
static esp_lcd_panel_handle_t panel_handle;
static struct ppg_task_params parameters_ppg_max30102;
static struct i2c_device max30102_device;
static struct i2c_device mpu6050_device;
static const char *TAG = "MAIN";
static const char *device_name = "ESP32_BLE";
static bool notify_enabled = false;

static void touch_sensor_task(void *pvParameter);
static void rtc_clock_task(void *pvParameter);
static void on_notify_state_changed(bool enabled);
static void on_time_write(current_time_t *time_data);

/* Send data to the website */
static void touch_sensor_task(void *pvParameter)
{
    uint16_t touch_value;
    touch_pad_init();
    touch_pad_config(TOUCH_PAD_NUM0, 0);

    while (1)
    {
        touch_pad_read(TOUCH_PAD_NUM0, &touch_value);
        if (touch_value < 500)
        {
            if (notify_enabled && ble_manager_is_connected())
            {
                float touch_value_send = 9.9f;
                ESP_LOGI(TAG, "Touch detected! Sending %.1f", touch_value_send);

                ble_manager_notify_message(
                    ble_manager_get_conn_handle(),
                    ble_manager_get_float32_char_handle(),
                    &touch_value_send,
                    sizeof(touch_value_send));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Get the current time from the website and set RTC */
static void on_time_write(current_time_t *time_data)
{
    ESP_LOGI(TAG, "Time received from client: %04d-%02d-%02d %02d:%02d:%02d",
             time_data->year, time_data->month, time_data->day,
             time_data->hours, time_data->minutes, time_data->seconds);

    /* Set the RTC with received time */
    struct tm timeinfo = {
        .tm_year = time_data->year - 1900,
        .tm_mon = time_data->month - 1,
        .tm_mday = time_data->day,
        .tm_hour = time_data->hours,
        .tm_min = time_data->minutes,
        .tm_sec = time_data->seconds,
        .tm_wday = time_data->day_of_week == 0 ? 0 : time_data->day_of_week - 1,
        .tm_isdst = -1};

    time_t t = mktime(&timeinfo);
    struct timeval tv = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&tv, NULL);

    ESP_LOGI(TAG, "RTC time set successfully");
}

/* Callback when notification subscription state changes */
static void on_notify_state_changed(bool enabled)
{
    notify_enabled = enabled;
    ESP_LOGI(TAG, "Notifications %s", enabled ? "enabled" : "disabled");
}

/* RTC Clock display task */
static void rtc_clock_task(void *pvParameter)
{
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];

    while (1)
    {
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        ESP_LOGI(TAG, "Current time: %s", strftime_buf);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void add_device_MAX30102(struct i2c_device *device)
{

    esp_err_t esp_ret;

    // setup device
    device->i2c_dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    device->i2c_dev_config.device_address = I2C_MAX30102_ADDR;
    device->i2c_dev_config.scl_speed_hz = I2C_FREQ_HZ;

    // initialize max30102_dev on the bus
    esp_ret = i2c_master_bus_add_device(i2c_bus_0, &device->i2c_dev_config, &device->i2c_dev_handle);
    if (esp_ret != ESP_OK)
    {
        DBG_PRINTF("Failed to add MAX30102 device to I2C bus: %d\n", esp_ret);
        abort();
    }
}

void add_device_SH1106(esp_lcd_panel_handle_t *panel_handle_tmp)
{
    *panel_handle_tmp = lcd_init(&i2c_bus_0);
}

void add_device_MPU6050(struct i2c_device *device)
{

    esp_err_t esp_ret;

    // setup device
    device->i2c_dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    device->i2c_dev_config.device_address = I2C_MPU6050_ADDR;
    device->i2c_dev_config.scl_speed_hz = I2C_FREQ_HZ;

    // initialize MPU6050 on the bus
    esp_ret = i2c_master_bus_add_device(i2c_bus_1, &device->i2c_dev_config, &device->i2c_dev_handle);
    if (esp_ret != ESP_OK)
    {
        DBG_PRINTF("Failed to add MPU6050 device to I2C bus: %d\n", esp_ret);
        abort();
    }
}

void PPG_sensor_task(void *parameters)
{
    // get parameters
    struct ppg_task_params *params = (struct ppg_task_params *)parameters;
    struct i2c_device *device = params->device;
    esp_err_t esp_ret;

    if (device == NULL)
    {
        DBG_PRINTF("DEVICE NULL\n");
    }

    esp_ret = init_multiled_mode(device, 0x0A, 0x1F, MAX30102_SPO2_RANGE_4096 | MAX30102_SPO2_50_SPS | MAX30102_SPO2_LED_PW_411);
    if (esp_ret != ESP_OK)
    {
        DBG_PRINTF("Failed to initialize multi-LED mode: %d\n", esp_ret);
        abort();
    }

    // set FIFO configuration: sample averaging = 4 (every sample is avg of 4 misurations), rollover enabled, almost full = 0
    esp_ret = max30102_set_register(device, MAX30102_FIFO_CFG_ADDR, MAX30102_SMP_AVE_NO | MAX30102_FIFO_ROLL_OVER | 0x0A);
    if (esp_ret != ESP_OK)
    {
        DBG_PRINTF("Failed to configure FIFO: %d\n", esp_ret);
        abort();
    }

    // Reset FIFO pointers
    esp_ret = reset_fifo_registers(device);
    if (esp_ret != ESP_OK)
    {
        DBG_PRINTF("Failed to reset FIFO registers: %d\n", esp_ret);
        abort();
    }

    uint8_t ovf_cntr = MAX30102_FIFO_OVF_CTR_ADDR;
    uint8_t wr_ptr;
    while (1)
    {
        // ESP_LOGI("PPG", "Remaining stack: %u bytes", uxTaskGetStackHighWaterMark(NULL));
        if (max30102_i2c_read_multiled_data_burst(device))
        {
            for (int i = 0; i < MAX30102_BPM_SAMPLES_SIZE; i++)
            {
                // DBG_PRINTF("%d - IR_RAW: %lu - IR_AC: %d\n", i, IR_buffer[i], IR_ac_buffer[i]);
                calculateBPM(IR_ac_buffer[i], &global_parameters.BPM, &global_parameters.AVG_BPM);
            }
            // ESP_LOGI("MAIN", "task_acc created: %s", retF == pdPASS ? "OK" : "FAIL");

            // TODO: create a task that handles sending the messages, so that the ppg_task does not get blocked
            if (notify_enabled && ble_manager_is_connected())
            {
                ESP_LOGI(TAG, "Sending IR_AC buffer");

                ble_manager_notify_message(
                    ble_manager_get_conn_handle(),
                    ble_manager_get_float32_char_handle(),
                    &IR_ac_buffer,
                    sizeof(int16_t) * MAX30102_BPM_SAMPLES_SIZE);
            }
        }
        // i2c_master_transmit_receive(device->i2c_dev_handle, &ovf_cntr, 1, &wr_ptr, 1, 1000);
        // DBG_PRINTF("overflow: %d\n",wr_ptr);
        // if(wr_ptr>=1){
        //     DBG_PRINTF("OVERFLOW!\n");
        // }

        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}

void LCD_task(void *parameters)
{
    global_parameters.show_heart = true;
    esp_lcd_panel_handle_t panel_handle = *(esp_lcd_panel_handle_t *)parameters;

    GPIO_init();
    memset(buffer_data, 0, sizeof(buffer_data));

    bool LCD_ON = false;
    EventType evt;

    while (1)
    {
        if (xQueueReceive(event_queue, &evt, portMAX_DELAY))
        {
            switch (evt)
            {
            case EVT_BUTTON_EDGE:
            {
                int level = gpio_get_level(PUSH_BUTTON_GPIO);
                if (level == 0) // pressed
                {
                    long_press_triggered = false;
                    xTimerStart(long_press_timer_handle, 0);
                }
                else // released
                {
                    xTimerStop(long_press_timer_handle, 0);
                    if (!long_press_triggered && LCD_ON) // short press
                    {
                        (*fsm[next_state].state_function)(&panel_handle, &global_parameters);

                        // Start/stop BPM animation if needed
                        if (current_state == STATE_BPM)
                            xTimerStart(frame_timer_handle, 0);
                        else
                            xTimerStop(frame_timer_handle, 0);
                    }
                }
                break;
            }

            case EVT_LONG_PRESS:
            {
                long_press_triggered = true;
                LCD_ON = !LCD_ON;

                if (LCD_ON)
                {
                    xTimerStart(refresh_timer_handle, 0);
                    TurnLcdOn(panel_handle);
                    current_state = STATE_BPM;
                    (*fsm[current_state].state_function)(&panel_handle, &global_parameters);
                    xTimerStart(frame_timer_handle, 0); // start animation
                }
                else
                {
                    TurnLcdOff(panel_handle);
                    xTimerStop(refresh_timer_handle, 0);
                    xTimerStop(frame_timer_handle, 0); // stop animation
                }
                break;
            }

            case EVT_REFRESH:
                (*fsm[current_state].state_function)(&panel_handle, &global_parameters);
                break;

            case EVT_FRAME:
                if (current_state == STATE_BPM)
                {
                    global_parameters.show_heart = !global_parameters.show_heart;
                    (*fsm[current_state].state_function)(&panel_handle, &global_parameters);
                    break;
                }

            default:
                break;
            }
        }
    }
}

void app_main()
{
    esp_task_wdt_deinit();

    // I2C busses init
    init_I2C_bus_PORT0(&i2c_bus_0);
    init_I2C_bus_PORT1(&i2c_bus_1);

    add_device_MAX30102(&max30102_device);
    add_device_MPU6050(&mpu6050_device);
    // add_device_SH1106 (&panel_handle);

    // parameters init
    parameters_ppg_max30102.bus = i2c_bus_0;
    parameters_ppg_max30102.device = &max30102_device;

    // BLE setup
    struct timeval tv = {.tv_sec = 0, .tv_usec = 0};
    settimeofday(&tv, NULL);

    if (ble_manager_init(device_name) != 0)
    {
        ESP_LOGE(TAG, "Failed to initialize BLE manager");
        return;
    }

    /* Register callbacks for BLE events */
    ble_manager_register_notify_state_cb(on_notify_state_changed);
    ble_manager_register_time_write_cb(on_time_write);

    TaskHandle_t ppg_task_handle = NULL;
    // tasks
    // xTaskCreate(
    //     LCD_task,
    //     "LCD_task_debug",
    //     4096,
    //     &panel_handle,
    //     1,
    //     NULL
    // );

    retF = xTaskCreatePinnedToCore(
        task_acc,
        "task_acc_debug",
        4096,
        &mpu6050_device,
        2,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        PPG_sensor_task,
        "PPG_sensor_task_debug",
        4096,
        &parameters_ppg_max30102,
        1,
        &ppg_task_handle,
        0);
    /* Start touch sensor task */
    // xTaskCreate(touch_sensor_task, "touch_sensor", 4096, NULL, 10, NULL);

    /* Start RTC clock display task */
    // xTaskCreate(rtc_clock_task, "rtc_clock", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Service initialized successfully");
}
