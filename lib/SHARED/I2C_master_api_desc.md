# Libreria `i2c_master.h` - Funzioni principali

Questo documento elenca le principali funzioni della libreria `driver/i2c_master.h` per ESP32 (I2C master), con una breve descrizione di ciascuna.

---

## Tipi principali

| Tipo | Descrizione |
|------|-------------|
| `i2c_master_bus_handle_t` | Handle per un bus I2C master. |
| `i2c_master_dev_handle_t` | Handle per un dispositivo slave sul bus master. |
| `i2c_master_bus_config_t` | Configurazione del bus (porta, SDA/SCL, glitch, pull-up). |
| `i2c_device_config_t` | Configurazione di un dispositivo I2C (indirizzo, velocità, lunghezza indirizzo). |
| `i2c_master_event_callbacks_t` | Struttura per callback di eventi master (es. `on_trans_done`). |

---

## Funzioni principali

| Funzione | Descrizione |
|----------|-------------|
| `i2c_new_master_bus(const i2c_master_bus_config_t *bus_config, i2c_master_bus_handle_t *ret_bus_handle)` | Crea un nuovo bus master. Restituisce un handle da usare per aggiungere device e trasmettere dati. |
| `i2c_master_bus_add_device(i2c_master_bus_handle_t bus_handle, const i2c_device_config_t *dev_config, i2c_master_dev_handle_t *ret_handle)` | Aggiunge un dispositivo slave al bus master. Restituisce un handle per comunicare con esso. |
| `i2c_master_transmit(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, int xfer_timeout_ms)` | Invia dati allo slave. `write_buffer` è il buffer dei dati, `write_size` la dimensione, `xfer_timeout_ms` il timeout. |
| `i2c_master_receive(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms)` | Legge dati dallo slave. `read_buffer` riceve i dati, `read_size` quanti byte leggere, `xfer_timeout_ms` timeout. |
| `i2c_master_transmit_receive(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms)` | Transazione combinata: scrive prima e legge poi. Utile per registri di dispositivi. |
| `i2c_master_probe(i2c_master_bus_handle_t bus_handle, uint32_t device_address, int xfer_timeout_ms)` | Prova a comunicare con un dispositivo a quell'indirizzo. Restituisce ESP_OK se il device è presente. |
| `i2c_master_bus_rm_device(i2c_master_dev_handle_t dev_handle)` | Rimuove un device dal bus master. |
| `i2c_del_master_bus(i2c_master_bus_handle_t bus_handle)` | Elimina un bus master, liberando le risorse hardware. |
| `i2c_master_get_bus_handle(i2c_port_t port, i2c_master_bus_handle_t *handle)` | Restituisce un handle del bus master già inizializzato per un dato port. |
| `i2c_master_register_event_callbacks(i2c_master_dev_handle_t i2c_dev, const i2c_master_event_callbacks_t *cbs, void *user_data)` | Registra callback per eventi della master (es. completamento trasmissione). |
| `i2c_master_multi_buffer_transmit(...)` | Trasmette più buffer in sequenza (scatter/gather), senza concatenare manualmente. |

---

### Nota
- Tutte le funzioni ritornano un codice di errore `esp_err_t` (`ESP_OK` se successo).  
- Per leggere documentazione completa ed esempi ufficiali: [Espressif IDF I2C Master](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2c.html)

