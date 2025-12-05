#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ble_manager.h"
#include "driver/touch_pad.h"
#include <sys/time.h>
#include <time.h>

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
    
    while (1) {
        touch_pad_read(TOUCH_PAD_NUM0, &touch_value);
        if (touch_value < 500) {
            if (notify_enabled && ble_manager_is_connected()) {
                float touch_value_send = 9.9f;
                ESP_LOGI(TAG, "Touch detected! Sending %.1f", touch_value_send);
                
                ble_manager_notify_time(
                    ble_manager_get_conn_handle(),
                    ble_manager_get_float32_char_handle(),
                    &touch_value_send,
                    sizeof(touch_value_send)
                );
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
        .tm_isdst = -1
    };
    
    time_t t = mktime(&timeinfo);
    struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
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
    
    while (1) {
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        ESP_LOGI(TAG, "Current time: %s", strftime_buf);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{   
    struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
    settimeofday(&tv, NULL);

    if (ble_manager_init(device_name) != 0) {
        ESP_LOGE(TAG, "Failed to initialize BLE manager");
        return;
    }

    /* Register callbacks for BLE events */
    ble_manager_register_notify_state_cb(on_notify_state_changed);
    ble_manager_register_time_write_cb(on_time_write);

    /* Start touch sensor task */
    xTaskCreate(touch_sensor_task, "touch_sensor", 2048, NULL, 10, NULL);
    
    /* Start RTC clock display task */
    xTaskCreate(rtc_clock_task, "rtc_clock", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "Service initialized successfully");
}
