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

// Clear GPR registers (including baseline data)
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

// Full reset procedure
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
    
    // Step 3: Clear GPR
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
    
    // Step 7: Wait for warm-up (3 minutes per datasheet)
    ESP_LOGI(TAG, "Waiting for warm-up period (3 minutes)...");
    for (int i = 0; i < 30; i++) // 180 seconds = 3 minutes
    {
        global_parameters.CO2_init_percentage = (i * 100) / 180; // Update global parameter for UI
        ESP_LOGI(TAG, "Warm-up progress: %d%%", (i * 100) / 180);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
    
    
    ESP_LOGI(TAG, "Full reset sequence completed");
    return ESP_OK;
}

esp_err_t ens160_init(i2c_master_bus_handle_t bus_handle) {
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "Invalid bus handle");
        return ESP_ERR_INVALID_ARG;
    }

    // Configure device with address (ADDR pin HIGH = 0x53)
    i2c_device_config_t ens160_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ENS160_ADDR_H,
        .scl_speed_hz = 100000,
    };
    
    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &ens160_dev_cfg, &ens160_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device at address 0x%02X: %s", 
                 ENS160_ADDR_L, esp_err_to_name(ret));
        return ret;
    }

    // Wait for sensor startup (datasheet specifies 100ms)
    vTaskDelay(pdMS_TO_TICKS(500));

    // Read Part ID to verify device presence
    uint8_t buf[2];
    ret = ens160_read_reg(ENS160_REG_PART_ID, buf, 2);
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

    // Perform FULL reset
    ret = ens160_full_reset();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Full reset failed");
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

    // Check data ready flag
    if (!(status & ENS160_STATUS_NEWDAT)) {
        ESP_LOGD(TAG, "Data not ready");
        return ESP_ERR_NOT_FINISHED;
    }
    
    // Check validity
    uint8_t validity = status & ENS160_STATUS_VALIDITY_MASK;
    if (validity == ENS160_STATUS_VALIDITY_INVALID) {
        ESP_LOGE(TAG, "Sensor output invalid");
        return ESP_ERR_INVALID_RESPONSE;
    } else if (validity == ENS160_STATUS_VALIDITY_WARMUP) {
        ESP_LOGW(TAG, "Sensor in warm-up phase");
    } else if (validity == ENS160_STATUS_VALIDITY_STARTUP) {
        ESP_LOGW(TAG, "Sensor in initial start-up phase");
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