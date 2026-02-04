#include "MPU6050_api.h"
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
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "../ENS160/ens160.h"

#include "mpu6050.h"
#include "roll_pitch.h"
#include "quaternions.h"
#define I2C_MASTER_SCL_IO 22 // SCL pin
#define I2C_MASTER_SDA_IO 21 // SDA pin
#define I2C_MASTER_FREQ_HZ 400000
#define I2C_MASTER_NUM I2C_NUM_0
#define ESP_INTR_FLAG_DEFAULT 0

struct ppg_task_params
{
    struct i2c_device *device;
    i2c_master_bus_handle_t bus;
};

BaseType_t retF;

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

void task_acc(void *parameters)
{
    esp_err_t ret;
    int16_t accel_x, accel_y, accel_z;
    int16_t gyro_x, gyro_y, gyro_z;

    float accel_x_g, accel_y_g, accel_z_g;
    float gyro_x_dps, gyro_y_dps, gyro_z_dps;

    float accel_bias[3] = {0};
    float gyro_bias[3] = {0};

    /* Calibrate */
    mpu6050_calibrate(accel_bias, gyro_bias);
    ESP_LOGI("MPU6050", "Calibration complete");

    /* Filters */
    roll_pitch_init();

    Quaternion q;
    quaternion_init(&q);

    while (1)
    {
        ret = mpu6050_read_raw_data(
            &accel_x, &accel_y, &accel_z,
            &gyro_x, &gyro_y, &gyro_z);

        if (ret != ESP_OK)
        {
            ESP_LOGE("MPU6050", "Read failed");
            continue; // <-- better than return inside a loop
        }
        else
        {
            mpu6050_convert_accel(
                accel_x, accel_y, accel_z,
                &accel_x_g, &accel_y_g, &accel_z_g);

            mpu6050_convert_gyro(
                gyro_x, gyro_y, gyro_z,
                &gyro_x_dps, &gyro_y_dps, &gyro_z_dps);

            roll_pitch_update(
                accel_x_g, accel_y_g, accel_z_g,
                gyro_x_dps, gyro_y_dps, gyro_z_dps);

            quaternion_update(
                &q,
                gyro_x_dps, gyro_y_dps, gyro_z_dps,
                accel_x_g, accel_y_g, accel_z_g,
                0.01f);

            // ESP_LOGI("ACC", "Accel: X=%.2f m/s^2, Y=%.2f m/s^2, Z=%.2f m/s^2",
            //          accel_x_g, accel_y_g, accel_z_g);

            // ESP_LOGI("ACC", "Gyro: X=%.2f deg/s, Y=%.2f deg/s, Z=%.2f deg/s",
            //          gyro_x_dps, gyro_y_dps, gyro_z_dps);

            // ESP_LOGI("ACC", "Roll: %.2f", roll_get());
            // ESP_LOGI("ACC", "Pitch: %.2f", pitch_get());

            // ESP_LOGI("ACC", "Quaternion Roll: %.2f", quaternion_get_roll(&q));
            // ESP_LOGI("ACC", "Quaternion Pitch: %.2f", quaternion_get_pitch(&q));
            // ESP_LOGI("ACC", "Quaternion Yaw: %.2f", quaternion_get_yaw(&q));
            // ESP_LOGI("ACC", "Quaternion x: %.2f", q.x);
            // ESP_LOGI("ACC", "Quaternion y: %.2f", q.y);
            // ESP_LOGI("ACC", "Quaternion z: %.2f", q.z);

            GyroData_t q_data = {
                .roll = roll_get(),
                .pitch = pitch_get(),
            };
            ESP_LOGI("ACC", "GyroData_t Roll: %.2f", q_data.roll);
            ESP_LOGI("ACC", "GyroData_t Pitch: %.2f", q_data.pitch);

            if (ble_manager_is_connected())
            {
                ble_manager_notify_gyro(
                    ble_manager_get_conn_handle(),
                    &q_data);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

/* Send data to the website */
// void gyro_ble_task(void *pvParameter)
// {
//     struct i2c_device *mpu_device = (struct i2c_device *)pvParameter;
//     Gyro_Axis_t gyro_raw = {0};
//     uint8_t gyro_buffer[6] = {0};

//     while (1)
//     {
//         if (notify_enabled && ble_manager_is_connected())
//         {

//             // mpu6050_read_reg(mpu_device, MPU6050_GYRO_XOUT_H, gyro_buffer, 6);

//             ble_manager_notify_gyro(
//                 ble_manager_get_conn_handle(),
//                 &gyro_raw);
//         }
//         vTaskDelay(pdMS_TO_TICKS(200));
//     }
// }

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
    char time_buf[64];

    while (1)
    {
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        ESP_LOGI(TAG, "Current time: %s", time_buf);

        int year, month, day, hour, minute, second;

        sscanf(time_buf, "%d-%d-%d %d:%d:%d",
               &year, &month, &day,
               &hour, &minute, &second);

        snprintf(global_parameters.date, sizeof(global_parameters.date), "%d/%d/%02d",
                 day,
                 month,
                 year % 100);
        snprintf(global_parameters.time_str, sizeof(global_parameters.time_str), "%02d:%02d", hour, minute);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void c02_check_task(void *pvParameter)
{
    ens160_data_t data;
    uint8_t consecutive_errors = 0;
    const uint8_t MAX_CONSECUTIVE_ERRORS = 3;

    while (1)
    {
        esp_err_t ret = ens160_read_data(&data);
        if (ret == ESP_OK)
        {
            consecutive_errors = 0;
            ESP_LOGI(TAG, "eCO2: %d ppm, TVOC: %d ppb, AQI: %d", data.eco2, data.tvoc, data.aqi);
            global_parameters.CO2 = data.eco2;
            global_parameters.CO2_risk_level = data.aqi;

            if (notify_enabled && ble_manager_is_connected())
            {
                ble_manager_notify_ens160(
                    ble_manager_get_conn_handle(),
                    &data);
            }
        }
        else
        {
            consecutive_errors++;
            ESP_LOGE(TAG, "Failed to read ENS160: %s (errors: %d/%d)",
                     esp_err_to_name(ret), consecutive_errors, MAX_CONSECUTIVE_ERRORS);

            if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS)
            {
                ESP_LOGW(TAG, "Performing ENS160 full reset");
                global_parameters.CO2 = 0;
                global_parameters.CO2_risk_level = 0;
                
                esp_err_t reset_ret = ens160_full_reset();
                if (reset_ret != ESP_OK)
                {
                    ESP_LOGE(TAG, "ENS160 Full reset failed: %s", esp_err_to_name(reset_ret));
                    ens160_deinit();
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    add_device_ENS160();
                }
                else
                {
                    ESP_LOGI(TAG, "ENS160 Full reset completed successfully");
                    consecutive_errors = 0;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
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
    device->i2c_dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7,
    device->i2c_dev_config.device_address = MPU6050_ADDR,
    device->i2c_dev_config.scl_speed_hz = 400000,

    esp_ret = i2c_master_bus_add_device(i2c_bus_1, &device->i2c_dev_config, &device->i2c_dev_handle);
    if (esp_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add ACC to bus");
    }
    mpu6050_set_handle(device->i2c_dev_handle);
}

esp_err_t add_device_ENS160()
{
    esp_err_t esp_ret;

    // initialize ENS160 on the bus
    esp_ret = ens160_init(i2c_bus_0);
    if (esp_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize ENS160: %s", esp_err_to_name(esp_ret));
    }
    return esp_ret;
}

void send_ppg_data_task(void *parameters)
{
    if (notify_enabled && ble_manager_is_connected())
    {
        // send bpm
        ESP_LOGI(TAG, "Sending BPM");
        ble_manager_notify_bpm(ble_manager_get_conn_handle(), global_parameters.BPM);
        // send avg bpm
        ESP_LOGI(TAG, "Sending AVG_BPM");
        ble_manager_notify_avgbpm(ble_manager_get_conn_handle(), global_parameters.AVG_BPM);
        // send IR filtered buffer
        ESP_LOGI(TAG, "Sending IR ac buffer");
        ble_manager_notify_iracbuffer(
            ble_manager_get_conn_handle(),
            ble_manager_get_iracbuffer_char_handle(),
            &IR_ac_buffer,
            sizeof(int16_t) * MAX30102_BPM_SAMPLES_SIZE);
        // send IR raw buffer
        ESP_LOGI(TAG, "Sending IR raw buffer");
        ble_manager_notify_irrawbuffer(
            ble_manager_get_conn_handle(),
            ble_manager_get_irrawbuffer_char_handle(),
            &IR_buffer,
            sizeof(uint32_t) * MAX30102_BPM_SAMPLES_SIZE);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    vTaskDelete(NULL);
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

            DBG_PRINTF("BPM: %d - AVG_BPM: %d \n", global_parameters.BPM, global_parameters.AVG_BPM);

            xTaskCreate(
                send_ppg_data_task,
                "send_PPG_data_BLE",
                4096,
                NULL,
                1,
                NULL);
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
    esp_lcd_panel_handle_t panel_handle = *(esp_lcd_panel_handle_t *)parameters;

    GPIO_init();
    memset(buffer_data, 0, sizeof(buffer_data));

    EventType evt;

    gpio_intr_disable(PUSH_BUTTON_GPIO);

    bool LCD_ON = true;
    TurnLcdOn(panel_handle);
    show_loading_screen(&panel_handle);

    xTimerStart(refresh_timer_handle, 0);
    global_parameters.show_heart = true;
    (*fsm[current_state].state_function)(&panel_handle, &global_parameters);
    xTimerStart(frame_timer_handle, 0);

    xQueueReset(event_queue);
    gpio_intr_enable(PUSH_BUTTON_GPIO);

    while (1)
    {
        if (xQueueReceive(event_queue, &evt, portMAX_DELAY))
        {
            switch (evt)
            {
            case EVT_GYRO:
            {
                LCD_ON = false;
                if (current_state == STATE_BPM)
                {
                    xTimerStop(frame_timer_handle, 0);
                }
                ESP_LOGE(TAG, "LCD OFF GYRO");
                TurnLcdOff(panel_handle);
                break;
            }
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
                        ESP_LOGE(TAG, "LCD NEXT STATE");
                        (*fsm[next_state].state_function)(&panel_handle, &global_parameters);

                        current_state = next_state;
                        next_state = get_next_state(current_state);

                        // Start/stop BPM animation if on BPM screen
                        if (current_state == STATE_BPM)
                        {
                            xTimerStart(frame_timer_handle, 0);
                        }
                        else
                        {
                            xTimerStop(frame_timer_handle, 0);
                        }
                    }
                }
                break;
            }

            case EVT_LONG_PRESS:
            {
                if (gpio_get_level(PUSH_BUTTON_GPIO) != 0)
                {
                    // Ignore stale long-press event if the button was already released.
                    break;
                }
                long_press_triggered = true;
                LCD_ON = !LCD_ON;

                if (LCD_ON)
                {
                    xTimerStart(refresh_timer_handle, 0);
                    TurnLcdOn(panel_handle);
                    ESP_LOGE(TAG, "LCD ON LONG PRESS");
                    current_state = STATE_BPM;
                    next_state = get_next_state(current_state);

                    global_parameters.show_heart = true;
                    (*fsm[current_state].state_function)(&panel_handle, &global_parameters);

                    xTimerStart(frame_timer_handle, 0); // start animation
                }
                else
                {
                    TurnLcdOff(panel_handle);
                    ESP_LOGE(TAG, "LCD OFF LONG PRESS");
                    xTimerStop(refresh_timer_handle, 0);
                    xTimerStop(frame_timer_handle, 0); // stop animation
                }
                break;
            }

            case EVT_REFRESH:
                (*fsm[current_state].state_function)(&panel_handle, &global_parameters);
                ESP_LOGE(TAG, "LCD REFRESH");
                break;

            default:
                break;
            }
        }
    }
}

static void bettery_level_task(void *pvParameter)
{
    // ADC init  GPIO32 -> ADC1_CHANNEL_4
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
    while (1)
    {
        printf("Checking battery level...\n");
        int val = adc1_get_raw(ADC1_CHANNEL_4);
        DBG_PRINTF("ADC1 CHANNEL 4 VALUE: %d\n", val);
        // Vout = Dout * Vmax(3.3V) / Dmax
        // V max for our battey is 4.2V but the circuit (voltage divider) attenuates it.
        // to get the max Vout the formulae is: Vout = Vin × R2 / (R1 + R2)
        // R1 = 10kohm, R2 = 10kohm Vin = 4.2V => Vout = 4.2 * 10k / (10k + 10k) = 2.1V
        float voltage = val * 3.3 / 4095;
        DBG_PRINTF("Voltage: %.2f V\n", voltage);
        global_parameters.battery_voltage = voltage; // reverse voltage divider
        // 4.2V=2.1V -> 100%
        // 3.95V= 1.97V -> 75%
        // 3.7V= 1.85V -> 50%
        // 3.5V= 1.75V -> 25%
        // 3.3V= 1.65V -> 0%

        if (voltage >= 1.95f)
            global_parameters.battery_state = BATTERY_FULL;
        else if (voltage >= 1.85f)
            global_parameters.battery_state = BATTERY_HIGH;
        else if (voltage >= 1.75f)
            global_parameters.battery_state = BATTERY_MEDIUM;
        else if (voltage >= 1.7f)
            global_parameters.battery_state = BATTERY_LOW;
        else
            global_parameters.battery_state = BATTERY_EMPTY;

        DBG_PRINTF("Battery: %d\n", global_parameters.battery_state);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

static uint64_t start_time;

void vConfigureTimerForRunTimeStats(void)
{
    start_time = esp_timer_get_time();
}

unsigned long ulGetRunTimeCounterValue(void)
{
    return (unsigned long)(esp_timer_get_time() - start_time);
}

void print_task_stats(void)
{
    // char buffer[2048];

    // printf("\n\n===== TASK LIST =====\n");
    // vTaskList(buffer);
    // printf("%s\n", buffer);

    // printf("\n===== RUNTIME STATS =====\n");
    // vTaskGetRunTimeStats(buffer);
    // printf("%s\n", buffer);
}

void app_main()
{
    // Enable detailed I2C logging to catch NACK sources
    /* esp_log_level_set("i2c", ESP_LOG_VERBOSE);
    esp_log_level_set("i2c_master", ESP_LOG_VERBOSE);
    esp_log_level_set("i2c.common", ESP_LOG_VERBOSE);
    esp_log_level_set("i2c.master", ESP_LOG_VERBOSE);     // questo è IL tag che ti interessa
    esp_log_level_set("lcd_panel.io.i2c", ESP_LOG_VERBOSE);
    esp_log_level_set("lcd_panel", ESP_LOG_VERBOSE);
    esp_log_level_set("*", ESP_LOG_INFO);  */
    // non alzare tutto a VERBOSE o diventi cieco

    /* esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret); */
    esp_task_wdt_deinit();

    // TODO: remove
    // Suppress NimBLE INFO logs (GATT procedure initiated, att_handle, etc.)
    esp_log_level_set("NimBLE", ESP_LOG_WARN);

    // I2C busses init
    init_I2C_bus_PORT0(&i2c_bus_0);
    init_I2C_bus_PORT1(&i2c_bus_1);

    add_device_SH1106(&panel_handle);
    vTaskDelay(pdMS_TO_TICKS(50));
    xTaskCreate(
        LCD_task,
        "LCD_task_debug",
        4096,
        &panel_handle,
        2,
        NULL);
    vTaskDelay(pdMS_TO_TICKS(500));

    add_device_MAX30102(&max30102_device);
    vTaskDelay(pdMS_TO_TICKS(500));
    add_device_MPU6050(&mpu6050_device);
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_err_t ens160_ret = add_device_ENS160();
    vTaskDelay(pdMS_TO_TICKS(500));

    // ppg parameters init
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

    retF = xTaskCreatePinnedToCore(
        task_acc,
        "task_acc_debug",
        8192,
        &mpu6050_device,
        1,
        NULL,
        1);
    vTaskDelay(pdMS_TO_TICKS(500));

    //* Start battery level monitoring task */
    xTaskCreate(
        bettery_level_task,
        "battery_level_task",
        2048,
        NULL,
        6,
        NULL);
    vTaskDelay(pdMS_TO_TICKS(500));
    /* Start RTC clock display task */
    xTaskCreate(rtc_clock_task, "rtc_clock", 4096, NULL, 4, NULL);
    vTaskDelay(pdMS_TO_TICKS(500));

    xTaskCreatePinnedToCore(
        PPG_sensor_task,
        "PPG_sensor_task_debug",
        4096,
        &parameters_ppg_max30102,
        3,
        &ppg_task_handle,
        0);
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Start CO2 check task */
    xTaskCreate(c02_check_task, "c02_check", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Service initialized successfully");

    /* Start touch sensor task */
    // xTaskCreate(touch_sensor_task, "touch_sensor", 4096, NULL, 10, NULL);
    while (1)
    {
        print_task_stats();
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_err_t err0 = i2c_master_probe(i2c_bus_0, I2C_MAX30102_ADDR, 0x7F);
        esp_err_t err1 = i2c_master_probe(i2c_bus_1, I2C_MPU6050_ADDR, 0x7F);
        esp_err_t err2 = i2c_master_probe(i2c_bus_0, 0x53, 0x7F);
        esp_err_t err3 = i2c_master_probe(i2c_bus_0, 0x52, 0x7F);

        printf("I2C probe results: MAX30102=%s, MPU6050=%s, ENS160=%s - %s\n",
               err0 == ESP_OK ? "OK" : "FAIL",
               err1 == ESP_OK ? "OK" : "FAIL",
               err2 == ESP_OK ? "OK 0x53" : "FAIL 0x53",
               err3 == ESP_OK ? "OK 0x52" : "FAIL 0x52");
    }
}
