#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "host/ble_gap.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <string.h>
#include "reg.h"

/* Device Custom Service UUID */
#define DEVICE_CUSTOM_SERVICE_UUID 0x1847
/* Current Time characteristic */
#define TIME_CHAR_UUID 0x2A2B
/* IRACBUFFER characteristic */
#define IRACBUFFER_CHAR_UUID 0x0014
/* Gyro characteristic */
#define GYRO_CHAR_UUID 0x0015
/* BPM characteristic */
#define BPM_CHAR_UUID 0x0016
/* AVGBPM characteristic */
#define AVGBPM_CHAR_UUID 0x0017
/* IRRAW characteristic */
#define IRRAWBUFFER_CHAR_UUID 0x0018

/* Current Time characteristic format */
typedef struct __attribute__((packed)) {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t day_of_week;  // 1=Monday, 7=Sunday, 0=Unknown
    uint8_t fractions256; // 1/256th of a second
    uint8_t adjust_reason; // Bit field for time adjustment reasons
} current_time_t;

/* Callback function types */
typedef void (*ble_notify_state_cb_t)(bool enabled);
typedef void (*ble_time_write_cb_t)(current_time_t *time_data);

/* Initialize BLE manager */
int ble_manager_init(const char *device_name);

/* Send notification with float32 data */
int ble_manager_notify_iracbuffer(uint16_t conn_handle, uint16_t char_handle, const void *data, uint16_t len);

/* Send notification with float32 data */
int ble_manager_notify_irrawbuffer(uint16_t conn_handle, uint16_t char_handle, const void *data, uint16_t len);

/* Send notification with Gyro_Axis_t data */
int ble_manager_notify_gyro(uint16_t conn_handle, const Gyro_Axis_t *gyro_data);

/* Send notification with BPM data */
int ble_manager_notify_bpm(uint16_t conn_handle, int16_t value);

/* Send notification with uint32_t data */
int ble_manager_notify_avgbpm(uint16_t conn_handle, int16_t value);

/* Get connection status */
bool ble_manager_is_connected(void);

/* Get current connection handle */
uint16_t ble_manager_get_conn_handle(void);

/* Get iracbuffer characteristic handle */
uint16_t ble_manager_get_iracbuffer_char_handle(void);
/* Get irraw characteristic handle */
uint16_t ble_manager_get_irrawbuffer_char_handle(void);

/* Get gyro characteristic handle */
uint16_t ble_manager_get_gyro_char_handle(void);

/* Get int16 characteristic handle */
uint16_t ble_manager_get_bpm_char_handle(void);

/* Get uint32 characteristic handle */
uint16_t ble_manager_get_avgbpm_char_handle(void);

/* Register callbacks */
void ble_manager_register_notify_state_cb(ble_notify_state_cb_t cb);
void ble_manager_register_time_write_cb(ble_time_write_cb_t cb);

#endif
