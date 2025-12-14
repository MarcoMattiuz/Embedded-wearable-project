#include "ble_manager.h"


static const char *TAG = "BLE_MANAGER";

/* BLE connection state */
static bool is_connected = false;
static uint16_t conn_handle = 0;
static uint8_t ble_addr_type;
static const char *device_name = NULL;

/* Characteristic handles */
static uint16_t iracbuffer_char_handle;
static uint16_t irrawbuffer_char_handle;
static uint16_t gyro_char_handle;
static uint16_t bpm_char_handle;
static uint16_t avgbpm_char_handle;

/* Callbacks */
static ble_notify_state_cb_t notify_state_callback = NULL;
static ble_time_write_cb_t time_write_callback = NULL;

/* Current float32 values for read operations */
static float current_iracbuffer_value = 0.0f;
static float current_irrawbuffer_value = 0.0f;
static Gyro_Axis_t current_gyro_value = {0};
static int16_t current_bpm_value = 0;
static uint32_t current_avgbpm_value = 0;

/* Function declarations */
static int ble_gap_event(struct ble_gap_event *event, void *arg);
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

/* GATT server definition */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Device Custom Service */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(DEVICE_CUSTOM_SERVICE_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
                /* Time Characteristic - writable by client */
                .uuid = BLE_UUID16_DECLARE(TIME_CHAR_UUID),
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_WRITE,
            }, {
                /* irac buffer Characteristic - notifiable to client */
                .uuid = BLE_UUID16_DECLARE(IRACBUFFER_CHAR_UUID),
                .access_cb = gatt_svr_chr_access,
                .val_handle = &iracbuffer_char_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            }, 
            {
                /* irraw buffer Characteristic - notifiable to client */
                .uuid = BLE_UUID16_DECLARE(IRRAWBUFFER_CHAR_UUID),
                .access_cb = gatt_svr_chr_access,
                .val_handle = &irrawbuffer_char_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            },
            {
                /* Gyro Characteristic - notifiable to client */
                .uuid = BLE_UUID16_DECLARE(GYRO_CHAR_UUID),
                .access_cb = gatt_svr_chr_access,
                .val_handle = &gyro_char_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            }, {
                /* BPM Characteristic - notifiable to client */
                .uuid = BLE_UUID16_DECLARE(BPM_CHAR_UUID),
                .access_cb = gatt_svr_chr_access,
                .val_handle = &bpm_char_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            }, {
                /* AVGBPM Characteristic - notifiable to client */
                .uuid = BLE_UUID16_DECLARE(AVGBPM_CHAR_UUID),
                .access_cb = gatt_svr_chr_access,
                .val_handle = &avgbpm_char_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            }, {
                0, /* No more characteristics */
            },
        }
    },
    {
        0, /* No more services */
    },
};

/* GATT characteristic access callback */
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                   struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == TIME_CHAR_UUID) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            /* Handle write from client (website) */
            uint16_t om_len;
            om_len = OS_MBUF_PKTLEN(ctxt->om);
            
            if (om_len == sizeof(current_time_t)) {
                current_time_t received_time;
                rc = ble_hs_mbuf_to_flat(ctxt->om, &received_time, sizeof(current_time_t), NULL);
                if (rc == 0) {
                    /* Call the time write callback if registered */
                    if (time_write_callback) {
                        time_write_callback(&received_time);
                    }
                    return 0;
                }
            }
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
    } else if (uuid == IRACBUFFER_CHAR_UUID) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &current_iracbuffer_value, sizeof(float));
            return rc == 0 ? 0 : BLE_ATT_ERR_UNLIKELY;
        }
    } 
    else if (uuid == IRRAWBUFFER_CHAR_UUID) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &current_irrawbuffer_value, sizeof(float));
            return rc == 0 ? 0 : BLE_ATT_ERR_UNLIKELY;
        }
    } 
    else if (uuid == GYRO_CHAR_UUID) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &current_gyro_value, sizeof(Gyro_Axis_t));
            return rc == 0 ? 0 : BLE_ATT_ERR_UNLIKELY;
        }
    } else if (uuid == BPM_CHAR_UUID) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &current_bpm_value, sizeof(int16_t));
            return rc == 0 ? 0 : BLE_ATT_ERR_UNLIKELY;
        }
    } else if (uuid == AVGBPM_CHAR_UUID) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &current_avgbpm_value, sizeof(int16_t));
            return rc == 0 ? 0 : BLE_ATT_ERR_UNLIKELY;
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}

/* GATT server registration callback */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "Registered service %s with handle=%d",
         ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
         ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG, "Registering characteristic %s with def_handle=%d val_handle=%d",
         ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
         ctxt->chr.def_handle,
         ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "Registering descriptor %s with handle=%d",
         ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
         ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

/* Initialize GATT server */
static int gatt_svr_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

/* Start advertising */
void ble_manager_start_advertising(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    memset(&fields, 0, sizeof(fields));

    /* Set advertising flags */
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /* Include TX power level */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    /* Set device name */
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;

    /* Set the UUIDs to advertise */
    fields.uuids16 = (ble_uuid16_t[]) {
        BLE_UUID16_INIT(DEVICE_CUSTOM_SERVICE_UUID)
    };
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error setting advertisement data; rc=%d", rc);
        return;
    }

    /* Begin advertising */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    
    rc = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error enabling advertisement; rc=%d", rc);
        return;
    }

    ESP_LOGI(TAG, "Advertising started");
}

/* GAP event handler */
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "Connection %s; status=%d",
                 event->connect.status == 0 ? "established" : "failed",
                 event->connect.status);

        if (event->connect.status == 0) {
            /* Connection established */
            conn_handle = event->connect.conn_handle;
            is_connected = true;
        } else {
            /* Connection failed; resume advertising */
            ble_manager_start_advertising();
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "Disconnect; reason=%d", event->disconnect.reason);
        
        is_connected = false;
        
        /* Resume advertising */
        ble_manager_start_advertising();
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "Advertisement complete");
        ble_manager_start_advertising();
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "Subscribe event; cur_notify=%d, attr_handle=%d",
                 event->subscribe.cur_notify, event->subscribe.attr_handle);
        
        if (event->subscribe.attr_handle == iracbuffer_char_handle) {
            bool notify_enabled = event->subscribe.cur_notify;
            
            if (notify_state_callback) {
                notify_state_callback(notify_enabled);
            }
        }

        if (event->subscribe.attr_handle == irrawbuffer_char_handle) {
            bool notify_enabled = event->subscribe.cur_notify;
            
            if (notify_state_callback) {
                notify_state_callback(notify_enabled);
            }
        }

        if (event->subscribe.attr_handle == bpm_char_handle) {
            bool notify_enabled = event->subscribe.cur_notify;
            
            if (notify_state_callback) {
                notify_state_callback(notify_enabled);
            }
        }

        if (event->subscribe.attr_handle == avgbpm_char_handle) {
            bool notify_enabled = event->subscribe.cur_notify;
            
            if (notify_state_callback) {
                notify_state_callback(notify_enabled);
            }
        }

        if (event->subscribe.attr_handle == gyro_char_handle) {
            bool notify_enabled = event->subscribe.cur_notify;
            
            if (notify_state_callback) {
                notify_state_callback(notify_enabled);
            }
        }
        break;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU update event; conn_handle=%d mtu=%d",
                 event->mtu.conn_handle, event->mtu.value);
        break;

    default:
        break;
    }

    return 0;
}

/* BLE host sync callback */
static void ble_on_sync(void)
{
    int rc;

    rc = ble_hs_id_infer_auto(0, &ble_addr_type);
    assert(rc == 0);

    ESP_LOGI(TAG, "BLE synchronized");

    /* Begin advertising */
    ble_manager_start_advertising();
}

/* BLE host reset callback */
static void ble_on_reset(int reason)
{
    ESP_LOGE(TAG, "Resetting state; reason=%d", reason);
}

/* BLE host task */
void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/* Initialize BLE manager */
int ble_manager_init(const char *dev_name)
{
    int rc;
    esp_err_t ret;

    device_name = dev_name;

    /* Initialize NimBLE */
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init nimble %d", ret);
        return ret;
    }

    /* Configure the host */
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;

    /* Initialize GATT server */
    rc = gatt_svr_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to init GATT server; rc=%d", rc);
        return rc;
    }

    /* Set the default device name */
    rc = ble_svc_gap_device_name_set(device_name);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set device name; rc=%d", rc);
        return rc;
    }

    /* Start the BLE host task */
    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "BLE Manager initialized successfully");
    
    return 0;
}

/* Send notification with float32 data */
int ble_manager_notify_iracbuffer(uint16_t conn_handle, uint16_t char_handle, const void *data, uint16_t len)
{
    struct os_mbuf *om;
    int rc;

    /* Update current value for read operations */
    if (len == sizeof(float) && char_handle == iracbuffer_char_handle) {
        memcpy(&current_iracbuffer_value, data, sizeof(float));
    }

    /* Create mbuf and send notification */
    om = ble_hs_mbuf_from_flat(data, len);
    if (om == NULL) {
        ESP_LOGE(TAG, "Error allocating mbuf");
        return -1;
    }

    rc = ble_gatts_notify_custom(conn_handle, char_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error sending notification; rc=%d", rc);
        return rc;
    }

    return 0;
}

/* Send notification with float32 data */
int ble_manager_notify_irrawbuffer(uint16_t conn_handle, uint16_t char_handle, const void *data, uint16_t len)
{
    struct os_mbuf *om;
    int rc;

    /* Update current value for read operations */
    if (len == sizeof(float) && char_handle == irrawbuffer_char_handle) {
        memcpy(&current_irrawbuffer_value, data, sizeof(float));
    }

    /* Create mbuf and send notification */
    om = ble_hs_mbuf_from_flat(data, len);
    if (om == NULL) {
        ESP_LOGE(TAG, "Error allocating mbuf");
        return -1;
    }

    rc = ble_gatts_notify_custom(conn_handle, char_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error sending notification; rc=%d", rc);
        return rc;
    }

    return 0;
}

/* Send notification with Gyro_Axis_t data */
int ble_manager_notify_gyro(uint16_t conn_handle, const Gyro_Axis_t *gyro_data)
{
    struct os_mbuf *om;
    int rc;

    /* Update current value for read operations */
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

/* Send notification with BPM data */
int ble_manager_notify_bpm(uint16_t conn_handle, int16_t value)
{
    struct os_mbuf *om;
    int rc;

    /* Update current value for read operations */
    current_bpm_value = value;

    om = ble_hs_mbuf_from_flat(&value, sizeof(int16_t));
    if (om == NULL) {
        ESP_LOGE(TAG, "Error allocating mbuf for int16");
        return -1;
    }

    rc = ble_gatts_notify_custom(conn_handle, bpm_char_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error sending int16 notification; rc=%d", rc);
        return rc;
    }

    return 0;
}

/* Send notification with AVGBPM data */
int ble_manager_notify_avgbpm(uint16_t conn_handle, int16_t value)
{
    struct os_mbuf *om;
    int rc;

    /* Update current value for read operations */
    current_avgbpm_value = value;

    om = ble_hs_mbuf_from_flat(&value, sizeof(int16_t));
    if (om == NULL) {
        ESP_LOGE(TAG, "Error allocating mbuf for uint32");
        return -1;
    }

    rc = ble_gatts_notify_custom(conn_handle, avgbpm_char_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error sending uint32 notification; rc=%d", rc);
        return rc;
    }

    return 0;
}

/* Get int16 characteristic handle */
uint16_t ble_manager_get_bpm_char_handle(void)
{
    return bpm_char_handle;
}

/* Get uint32 characteristic handle */
uint16_t ble_manager_get_avgbpm_char_handle(void)
{
    return avgbpm_char_handle;
}

/* Get gyro characteristic handle */
uint16_t ble_manager_get_gyro_char_handle(void)
{
    return gyro_char_handle;
}

/* Get connection status */
bool ble_manager_is_connected(void)
{
    return is_connected;
}

/* Get current connection handle */
uint16_t ble_manager_get_conn_handle(void)
{
    return conn_handle;
}

/* Get float32 characteristic handle */
uint16_t ble_manager_get_iracbuffer_char_handle(void)
{
    return iracbuffer_char_handle;
}

uint16_t ble_manager_get_irrawbuffer_char_handle(void)
{
    return irrawbuffer_char_handle;
}

/* Register callbacks */
void ble_manager_register_notify_state_cb(ble_notify_state_cb_t cb)
{
    notify_state_callback = cb;
}

void ble_manager_register_time_write_cb(ble_time_write_cb_t cb)
{
    time_write_callback = cb;
}
