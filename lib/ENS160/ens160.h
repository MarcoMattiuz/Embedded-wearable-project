#ifndef ENS160_H
#define ENS160_H

#include "driver/i2c_master.h"
#include "esp_err.h"

/* I2C address when ADDR pin is LOW */
#define ENS160_ADDR_L                   0x52
/* I2C address when ADDR pin is HIGH */
#define ENS160_ADDR_H                   0x53

/* Expected Part ID value for ENS160 */
#define ENS160_PART_ID_VAL              0x0160

/* Part ID register (2 bytes) */
#define ENS160_REG_PART_ID              0x00
/* Operating mode register */
#define ENS160_REG_OPMODE               0x10
/* Configuration register */
#define ENS160_REG_CONFIG               0x11
/* Command register */
#define ENS160_REG_COMMAND              0x12
/* Temperature input register (2 bytes) */
#define ENS160_REG_TEMP_IN              0x13
/* Relative humidity input register (2 bytes) */
#define ENS160_REG_RH_IN                0x15
/* Data status register */
#define ENS160_REG_DATA_STATUS          0x20
/* Air Quality Index data register */
#define ENS160_REG_DATA_AQI             0x21
/* TVOC data register (2 bytes) */
#define ENS160_REG_DATA_TVOC            0x22
/* eCO2 data register (2 bytes) */
#define ENS160_REG_DATA_ECO2            0x24
/* General Purpose Write registers */
#define ENS160_REG_GPR_WRITE            0x40
/* General Purpose Read registers */
#define ENS160_REG_GPR_READ             0x48

/* Deep sleep mode (lowest power consumption) */
#define ENS160_OPMODE_DEEP_SLEEP        0x00
/* Idle mode (heater off, no measurements) */
#define ENS160_OPMODE_IDLE              0x01
/* Standard mode (continuous measurements) */
#define ENS160_OPMODE_STD               0x02
/* Reset mode (software reset) */
#define ENS160_OPMODE_RESET             0xF0

/* Commands (written to COMMAND register in IDLE mode) */
#define ENS160_COMMAND_NOP              0x00
#define ENS160_COMMAND_GET_APPVER       0x0E
#define ENS160_COMMAND_CLRGPR           0xCC

/* New data available flag */
#define ENS160_STATUS_NEWDAT            0x02
/* New general purpose read data available */
#define ENS160_STATUS_NEWGPR            0x01
/* Validity status masks */
#define ENS160_STATUS_VALIDITY_MASK     0x0C
#define ENS160_STATUS_VALIDITY_NORMAL   0x00
#define ENS160_STATUS_VALIDITY_WARMUP   0x04
#define ENS160_STATUS_VALIDITY_STARTUP  0x08
#define ENS160_STATUS_VALIDITY_INVALID  0x0C

/* I2C communication timeout in milliseconds */
#define ENS160_I2C_TIMEOUT_MS           1000

/* 
* ENS160 sensor data structure 
* eco2: Equivalent CO2 concentration in ppm (400-65000)
* tvoc: Total Volatile Organic Compounds in ppb (0-65000)
* aqi: Air Quality Index (1=Excellent, 2=Good, 3=Moderate, 4=Poor, 5=Unhealthy)
*/
typedef struct {
    uint16_t eco2;
    uint16_t tvoc;
    uint8_t aqi;
} ens160_data_t;

/*
 * Initialize the ENS160 sensor
 * - Adds the device to the I2C bus
 * - Verifies the Part ID
 * - Sets the operating mode to Standard (continuous measurements)
 * - Waits for the first measurement to be ready
 * 
 * Returns:
 *   ESP_OK: Success
 *   ESP_ERR_INVALID_ARG: Invalid bus handle
 *   ESP_ERR_NOT_FOUND: Part ID verification failed
 *   ESP_FAIL: Operation mode setting failed
 *   Other ESP_ERR codes from I2C operations
 * 
 * The sensor requires approximately 1100ms to complete initialization
 * (100ms startup + 1000ms for first measurement in STD mode)
 */
esp_err_t ens160_init(i2c_master_bus_handle_t bus_handle);

/*
 * Read measurement data from ENS160 sensor
 * Reads the current air quality measurements from the sensor,
 * 
 * Returns:
 *   ESP_OK: Success, data read successfully
 *   ESP_ERR_INVALID_ARG: NULL data pointer
 *   ESP_ERR_INVALID_STATE: Sensor not initialized
 *   ESP_ERR_NOT_FINISHED: Data not ready yet
 *   Other ESP_ERR codes from I2C operations
 * 
 * New data is available every 1000ms in STD mode
 */
esp_err_t ens160_read_data(ens160_data_t *data);

/*
 * Perform full reset including NVM baseline clear
 * This clears the sensor's internal baseline data stored in NVM,
 * preventing persistent failure states across ESP32 restarts.
 * 
 * Returns:
 *   ESP_OK: Success
 *   ESP_ERR_INVALID_STATE: Sensor not initialized
 *   Other ESP_ERR codes from I2C operations
 */
esp_err_t ens160_full_reset(void);

/* Deinitialize the ENS160 sensor */
esp_err_t ens160_deinit(void);

/* Get the ENS160 device handle */
i2c_master_dev_handle_t ens160_get_dev_handle(void);

#endif