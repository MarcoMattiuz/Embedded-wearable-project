#ifndef ENS160_H
#define ENS160_H

#include "driver/i2c_master.h"
#include "esp_err.h"

// ENS160 I2C addresses
#define ENS160_ADDR_L                   0x52
#define ENS160_ADDR_H                   0x53

// ENS160 Part ID
#define ENS160_PART_ID_VAL              0x0160

// Registers
#define ENS160_REG_PART_ID              0x00
#define ENS160_REG_OPMODE               0x10
#define ENS160_REG_CONFIG               0x11
#define ENS160_REG_COMMAND              0x12
#define ENS160_REG_TEMP_IN              0x13
#define ENS160_REG_RH_IN                0x15
#define ENS160_REG_DATA_STATUS          0x20
#define ENS160_REG_DATA_AQI             0x21
#define ENS160_REG_DATA_TVOC            0x22
#define ENS160_REG_DATA_ECO2            0x24

// Operating modes
#define ENS160_OPMODE_DEEP_SLEEP        0x00
#define ENS160_OPMODE_IDLE              0x01
#define ENS160_OPMODE_STD               0x02
#define ENS160_OPMODE_RESET             0xF0

// Status bits
#define ENS160_STATUS_NEWDAT            0x02
#define ENS160_STATUS_NEWGPR            0x01

// Timeouts
#define ENS160_I2C_TIMEOUT_MS           1000

typedef struct {
    uint16_t eco2;  // ppm
    uint16_t tvoc;  // ppb
    uint8_t aqi;    // 1-5
} ens160_data_t;

esp_err_t ens160_init(i2c_master_bus_handle_t bus_handle);
esp_err_t ens160_read_data(ens160_data_t *data);
esp_err_t ens160_deinit(void);

#endif