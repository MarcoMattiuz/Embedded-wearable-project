### Questo e' un esempio di una characteristic che l'esp invia al client

## Per quasi tutti gli snippet, trova dove si trova nel codice, copialo e rinominalo

---

### ble_manager.h

```c
// Incrementa di 1 (es. #define NEW_CHAR_UUID 0x0016)
// Gyro characteristic
#define GYRO_CHAR_UUID 0x0015

// Send notification with Gyro_Axis_t data
int ble_manager_notify_gyro(uint16_t conn_handle, const Gyro_Axis_t *gyro_data);

// Get gyro characteristic handle
uint16_t ble_manager_get_gyro_char_handle(void);
```


---

### ble_manager.c

```c
// Characteristic handles
static uint16_t gyro_char_handle;

// Current float32 values for read operations
static Gyro_Axis_t current_gyro_value = {0};

// GATT server definition
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    // ...
    {
        // Gyro Characteristic - notifiable to client
        .uuid = BLE_UUID16_DECLARE(GYRO_CHAR_UUID),
        .access_cb = gatt_svr_chr_access,
        .val_handle = &gyro_char_handle,
        .flags = BLE_GATT_CHR_F_NOTIFY,
    },
    // ...
};

// GATT characteristic access callback
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    // ...
    if (uuid == GYRO_CHAR_UUID) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &current_gyro_value, sizeof(Gyro_Axis_t));
            return rc == 0 ? 0 : BLE_ATT_ERR_UNLIKELY;
        }
    }
    // ...
}

// Send notification with Gyro_Axis_t data
int ble_manager_notify_gyro(uint16_t conn_handle, const Gyro_Axis_t *gyro_data)
{
    struct os_mbuf *om;
    int rc;

    // Update current value for read operations
    memcpy(&current_gyro_value, gyro_data, sizeof(Gyro_Axis_t));

    om = ble_hs_mbuf_from_flat(gyro_data, sizeof(Gyro_Axis_t));
    if (om == NULL) {
        ESP_LOGE(TAG, "Error allocating mbuf for gyro");
        return -1;
    }

    rc = ble_gatts_notify_custom(conn_handle, gyro_char_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error sending gyro notification; rc=%d", rc);
        return rc;
    }

    return 0;
}

// Get gyro characteristic handle
uint16_t ble_manager_get_gyro_char_handle(void)
{
    return gyro_char_handle;
}
```


---

### script.js

```js
const GYRO_CHAR_UUID = 0x0015;

let gyroCharacteristic = null;

async function toggleConnection() {
    // ...
    gyroCharacteristic = await service.getCharacteristic(GYRO_CHAR_UUID);
    await gyroCharacteristic.startNotifications();
    gyroCharacteristic.addEventListener(
        "characteristicvaluechanged",
        handleGyroNotification
    );
}

// nel caso sia piu di una variabile basta splittare la stringa come in questo caso
function handleGyroNotification(event) {
    const value = event.target.value;
    if (value.byteLength >= 6) {
        const gx = value.getInt16(0, true);
        const gy = value.getInt16(2, true);
        const gz = value.getInt16(4, true);
        log(`Gyro: X=${gx} Y=${gy} Z=${gz}`);
    }
}
```

### main.c
```c
// Send data to the website
void gyro_ble_task(void *pvParameter)
{
    struct i2c_device *mpu_device = (struct i2c_device *)pvParameter;
    Gyro_Axis_t gyro_raw = {0};
    uint8_t gyro_buffer[6] = {0};

    while (1)
    {
        if (notify_enabled && ble_manager_is_connected())
        {
            mpu6050_read_reg(mpu_device, MPU6050_GYRO_XOUT_H, gyro_buffer, 6);

            ble_manager_notify_gyro(
                ble_manager_get_conn_handle(),
                &gyro_raw);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// Start Gyro BLE notification task
// xTaskCreatePinnedToCore(gyro_ble_task, "gyro_ble_task", 4096, &mpu6050_device, 1, NULL, 0);
```










