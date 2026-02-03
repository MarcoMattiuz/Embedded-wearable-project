#include "ens160.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ENS160";
static i2c_master_dev_handle_t ens160_dev_handle = NULL;

static esp_err_t ens160_write_reg(uint8_t reg, uint8_t value) {
    uint8_t cmd[] = {reg, value};
    esp_err_t ret = i2c_master_transmit(ens160_dev_handle, cmd, sizeof(cmd), 
                                        ENS160_I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write register 0x%02X: %s", reg, esp_err_to_name(ret));
    }
    return ret;
}

static esp_err_t ens160_read_reg(uint8_t reg, uint8_t *data, size_t len) {
    esp_err_t ret = i2c_master_transmit_receive(ens160_dev_handle, &reg, 1, data, len, 
                                                 ENS160_I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read register 0x%02X: %s", reg, esp_err_to_name(ret));
    }
    return ret;
}

static esp_err_t ens160_set_opmode(uint8_t opmode) {
    esp_err_t ret = ens160_write_reg(ENS160_REG_OPMODE, opmode);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Verify opmode was set
    vTaskDelay(pdMS_TO_TICKS(20));
    uint8_t read_opmode;
    ret = ens160_read_reg(ENS160_REG_OPMODE, &read_opmode, 1);
    if (ret == ESP_OK && read_opmode != opmode) {
        ESP_LOGE(TAG, "Opmode verification failed. Expected 0x%02X, got 0x%02X", 
                 opmode, read_opmode);
        return ESP_FAIL;
    }
    
    return ret;
}

esp_err_t ens160_init(i2c_master_bus_handle_t bus_handle) {
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "Invalid bus handle");
        return ESP_ERR_INVALID_ARG;
    }

    // Try both possible I2C addresses (ADDR pin LOW = 0x52, HIGH = 0x53)
    const uint8_t addresses[] = {ENS160_ADDR_L, ENS160_ADDR_H};
    esp_err_t ret = ESP_ERR_NOT_FOUND;
    
    for (int i = 0; i < 2; i++) {
        i2c_device_config_t ens160_dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addresses[i],
            .scl_speed_hz = 100000,
        };
        
        ret = i2c_master_bus_add_device(bus_handle, &ens160_dev_cfg, &ens160_dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to add device at address 0x%02X: %s", 
                     addresses[i], esp_err_to_name(ret));
            continue;
        }

        // Wait for sensor startup (datasheet specifies 100ms)
        vTaskDelay(pdMS_TO_TICKS(100));

        // Try to read Part ID to verify device presence
        uint8_t buf[2];
        ret = ens160_read_reg(ENS160_REG_PART_ID, buf, 2);
        if (ret == ESP_OK) {
            uint16_t part_id = (buf[1] << 8) | buf[0];
            if (part_id == ENS160_PART_ID_VAL) {
                ESP_LOGI(TAG, "ENS160 found at address 0x%02X, Part ID: 0x%04X", 
                         addresses[i], part_id);
                break;  // Device found successfully
            }
            ESP_LOGW(TAG, "Part ID mismatch at 0x%02X. Expected 0x%04X, got 0x%04X", 
                     addresses[i], ENS160_PART_ID_VAL, part_id);
        } else {
            ESP_LOGW(TAG, "No response at address 0x%02X", addresses[i]);
        }
        
        // Remove device and try next address
        i2c_master_bus_rm_device(ens160_dev_handle);
        ens160_dev_handle = NULL;
        ret = ESP_ERR_NOT_FOUND;
    }

    if (ret != ESP_OK || ens160_dev_handle == NULL) {
        ESP_LOGE(TAG, "ENS160 not found at any address. Check wiring and ADDR pin.");
        return ESP_ERR_NOT_FOUND;
    }

    // Set operating mode to standard
    ret = ens160_set_opmode(ENS160_OPMODE_STD);
    if (ret != ESP_OK) {
        ens160_deinit();
        return ret;
    }

    // Wait for initial measurement
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "ENS160 initialized successfully");
    return ESP_OK;
}

esp_err_t ens160_read_data(ens160_data_t *data) {
    if (data == NULL) {
        ESP_LOGE(TAG, "NULL data pointer");
        return ESP_ERR_INVALID_ARG;
    }

    if (ens160_dev_handle == NULL) {
        ESP_LOGE(TAG, "ENS160 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Check data status
    uint8_t status;
    esp_err_t ret = ens160_read_reg(ENS160_REG_DATA_STATUS, &status, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[ENS160] Failed to read status register");
        return ret;
    }

    ESP_LOGI(TAG, "Status register: 0x%02X (NEWDAT=%d, NEWGPR=%d, VALIDITY=%d)", 
             status, (status >> 1) & 1, status & 1, (status >> 2) & 3);
    if (!(status & ENS160_STATUS_NEWDAT)) {
        ESP_LOGE(TAG, "Data not ready");
        return ESP_ERR_NOT_FINISHED;
    }

    // Read eCO2 (2 bytes, little-endian)
    uint8_t buf[2];
    ret = ens160_read_reg(ENS160_REG_DATA_ECO2, buf, 2);
    if (ret != ESP_OK) {
        return ret;
    }
    data->eco2 = (buf[1] << 8) | buf[0];

    // Read TVOC (2 bytes, little-endian)
    ret = ens160_read_reg(ENS160_REG_DATA_TVOC, buf, 2);
    if (ret != ESP_OK) {
        return ret;
    }
    data->tvoc = (buf[1] << 8) | buf[0];

    // Read AQI (1 byte)
    ret = ens160_read_reg(ENS160_REG_DATA_AQI, &data->aqi, 1);
    if (ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

esp_err_t ens160_deinit(void) {
    if (ens160_dev_handle != NULL) {
        esp_err_t ret = i2c_master_bus_rm_device(ens160_dev_handle);
        ens160_dev_handle = NULL;
        return ret;
    }
    return ESP_OK;
}