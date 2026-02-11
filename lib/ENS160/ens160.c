#include "ens160.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ENS160";
static i2c_master_dev_handle_t ens160_dev_handle = NULL;

void ens160_set_handle(i2c_master_dev_handle_t handle) {
    ens160_dev_handle = handle;
}

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
    
    // Wait for mode change to take effect
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Verify opmode was set
    uint8_t read_opmode;
    ret = ens160_read_reg(ENS160_REG_OPMODE, &read_opmode, 1);
    if (ret == ESP_OK && read_opmode != opmode) {
        ESP_LOGE(TAG, "Opmode verification failed. Expected 0x%02X, got 0x%02X", 
                 opmode, read_opmode);
        return ESP_FAIL;
    }
    
    return ret;
}

static esp_err_t ens160_clear_gpr(void) {
    esp_err_t ret;
    
    // Must be in IDLE mode to issue commands
    ret = ens160_set_opmode(ENS160_OPMODE_IDLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enter IDLE mode for GPR clear");
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Issue clear GPR command
    ret = ens160_write_reg(ENS160_REG_COMMAND, ENS160_COMMAND_CLRGPR);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send CLRGPR command");
        return ret;
    }
    
    // Wait for command to complete
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "GPR registers cleared successfully");
    return ESP_OK;
}

esp_err_t ens160_full_reset(void) {
    if (ens160_dev_handle == NULL) {
        ESP_LOGE(TAG, "Device not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Starting full reset sequence...");
    
    // Step 1: Go to DEEP SLEEP to fully disable sensor
    ret = ens160_set_opmode(ENS160_OPMODE_DEEP_SLEEP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enter DEEP_SLEEP");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Step 2: Go to IDLE mode
    ret = ens160_set_opmode(ENS160_OPMODE_IDLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enter IDLE");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Step 3: Clear GPR (baseline data)
    ret = ens160_clear_gpr();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear GPR");
        return ret;
    }
    
    // Step 4: Software reset
    ret = ens160_write_reg(ENS160_REG_OPMODE, ENS160_OPMODE_RESET);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to issue software reset");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Step 5: Return to IDLE
    ret = ens160_set_opmode(ENS160_OPMODE_IDLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to return to IDLE after reset");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Step 6: Enter STANDARD mode
    ret = ens160_set_opmode(ENS160_OPMODE_STD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enter STANDARD mode");
        return ret;
    }
    
    // Step 7: Wait for sensor readiness
    int elapsed_sec = 0;
    const int MAX_TIMEOUT_SEC = 500;
    const int PERCENTAGE_INTERVAL_SEC = 5;
    bool ready = false;

    while (elapsed_sec < MAX_TIMEOUT_SEC) {
        uint8_t status = 0;
        ret = ens160_read_reg(ENS160_REG_DATA_STATUS, &status, 1);
        
        if (ret == ESP_OK) {
            // Check Validity Flag (Bits 2:3)
            // 00 = Normal Operation
            // 01 = Warm-Up
            // 10 = Initial Start-Up
            // 11 = Invalid
            uint8_t validity = status & ENS160_STATUS_VALIDITY_MASK;

            if (validity == ENS160_STATUS_VALIDITY_NORMAL) {
                ESP_LOGI(TAG, "Sensor Ready! Status: 0x%02X", status);
                ready = true;
                break;
            }
        } else {
            ESP_LOGW(TAG, "Failed to read status during warm-up");
        }

        // Every 5 seconds, increase percentage by 1
        if (elapsed_sec > 0 && (elapsed_sec % PERCENTAGE_INTERVAL_SEC) == 0) {
            int percentage = elapsed_sec / PERCENTAGE_INTERVAL_SEC;
            if (percentage > 99) percentage = 99;
            global_parameters.CO2_init_percentage = percentage;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
        elapsed_sec++;
    }

    if (ready) {
        global_parameters.CO2_init_percentage = 100;
        ESP_LOGI(TAG, "Full reset sequence completed successfully");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Sensor warm-up timed out after %d seconds", MAX_TIMEOUT_SEC);
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t ens160_init(i2c_master_bus_handle_t bus_handle) {
    // Wait for sensor startup (datasheet specifies 100ms)
    vTaskDelay(pdMS_TO_TICKS(500));

    // Read Part ID to verify device presence
    uint8_t buf[2];
    esp_err_t ret = ens160_read_reg(ENS160_REG_PART_ID, buf, 2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read Part ID");
        i2c_master_bus_rm_device(ens160_dev_handle);
        ens160_dev_handle = NULL;
        return ret;
    }
    
    uint16_t part_id = (buf[1] << 8) | buf[0];
    if (part_id != ENS160_PART_ID_VAL) {
        ESP_LOGE(TAG, "Part ID mismatch. Expected 0x%04X, got 0x%04X", 
                 ENS160_PART_ID_VAL, part_id);
        i2c_master_bus_rm_device(ens160_dev_handle);
        ens160_dev_handle = NULL;
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "ENS160 found, Part ID: 0x%04X", part_id);

    // Step 1: Set to IDLE to ensure clean state
    ret = ens160_set_opmode(ENS160_OPMODE_IDLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enter IDLE mode");
        ens160_deinit();
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Step 2: Set to STANDARD mode to start measuring
    ret = ens160_set_opmode(ENS160_OPMODE_STD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enter STANDARD mode");
        ens160_deinit();
        return ret;
    }
    
    ESP_LOGI(TAG, "ENS160 initialized successfully");
    return ESP_OK;
}

esp_err_t ens160_read_data(ens160_data_t *data) {
    if (data == NULL) {
        ESP_LOGE(TAG, "NULL data pointer");
        return ESP_ERR_INVALID_ARG;
    }

    if (ens160_dev_handle == NULL) {
        ESP_LOGE(TAG, "ENS160 not initialized - handle is NULL");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGD(TAG, "ENS160 handle valid: %p", (void*)ens160_dev_handle);

    // Check data status
    uint8_t status;
    esp_err_t ret = ens160_read_reg(ENS160_REG_DATA_STATUS, &status, 1);
    if (ret != ESP_OK) {
        return ret;
    }

    // Check validity flags
    uint8_t validity = status & ENS160_STATUS_VALIDITY_MASK;
    
    if (validity == ENS160_STATUS_VALIDITY_INVALID) {
        ESP_LOGE(TAG, "Sensor output invalid");
        return ESP_ERR_INVALID_RESPONSE;
    } 
    else if (validity == ENS160_STATUS_VALIDITY_WARMUP) {
        ESP_LOGW(TAG, "Sensor in warm-up phase");
        return ESP_ERR_NOT_FINISHED; 
    } 
    else if (validity == ENS160_STATUS_VALIDITY_STARTUP) {
        ESP_LOGW(TAG, "Sensor in initial start-up phase");
        return ESP_ERR_NOT_FINISHED;
    }

    // Check data ready flag for normal operation
    if (!(status & ENS160_STATUS_NEWDAT)) {
        ESP_LOGD(TAG, "Data not ready");
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

    // Read AQI
    ret = ens160_read_reg(ENS160_REG_DATA_AQI, &data->aqi, 1);
    if (ret != ESP_OK) {
        return ret;
    }
    data->aqi &= 0x07;  // Only lower 3 bits are valid
    
    return ret;
}

esp_err_t ens160_deinit(void) {
    if (ens160_dev_handle != NULL) {
        // Put sensor in deep sleep before removing
        ens160_set_opmode(ENS160_OPMODE_DEEP_SLEEP);
        vTaskDelay(pdMS_TO_TICKS(50));
        
        esp_err_t ret = i2c_master_bus_rm_device(ens160_dev_handle);
        ens160_dev_handle = NULL;
        return ret;
    }
    return ESP_OK;
}

i2c_master_dev_handle_t ens160_get_dev_handle(void) {
    return ens160_dev_handle;
}