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

    i2c_device_config_t dev_cfg = {
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = 400000,
    };

    ret = i2c_master_bus_add_device(i2c_bus_1, &dev_cfg, &mpu6050_dev);
    
    mpu6050_set_handle(mpu6050_dev);

    if (ret != ESP_OK)
        ESP_LOGE("MPU6050", "add to bus failed");

    /* Wake up MPU6050 */
    uint8_t cmd[2] = {
        MPU6050_REG_PWR_MGMT_1,
        0x00};

    i2c_master_transmit(mpu6050_dev, cmd, sizeof(cmd), -1);


    if (ret != ESP_OK)
        ESP_LOGE("MPU6050", "master transmit failed");

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

        ESP_LOGI("ACC", "Accel: X=%.2f m/s^2, Y=%.2f m/s^2, Z=%.2f m/s^2",
                 accel_x_g, accel_y_g, accel_z_g);

        ESP_LOGI("ACC", "Gyro: X=%.2f deg/s, Y=%.2f deg/s, Z=%.2f deg/s",
                 gyro_x_dps, gyro_y_dps, gyro_z_dps);

        ESP_LOGI("ACC", "Roll: %.2f", roll_get());
        ESP_LOGI("ACC", "Pitch: %.2f", pitch_get());

        ESP_LOGI("ACC", "Quaternion Roll: %.2f", quaternion_get_roll(&q));
        ESP_LOGI("ACC", "Quaternion Pitch: %.2f", quaternion_get_pitch(&q));
        ESP_LOGI("ACC", "Quaternion Yaw: %.2f", quaternion_get_yaw(&q));

        vTaskDelay(pdMS_TO_TICKS(200)); // <- modern FreeRTOS macro
    }
}
void app_main()
{
    init_I2C_bus_PORT1(&i2c_bus_1);
    esp_task_wdt_deinit();

    // TODO: remove
    // Suppress NimBLE INFO logs (GATT procedure initiated, att_handle, etc.)
    esp_log_level_set("NimBLE", ESP_LOG_WARN);

    

    retF = xTaskCreatePinnedToCore(
        task_acc,
        "task_acc_debug",
        4096,
        &mpu6050_device,
        1,
        NULL,
        1);
    vTaskDelay(pdMS_TO_TICKS(1000));


    ESP_LOGI(TAG, "Service initialized successfully");
}
