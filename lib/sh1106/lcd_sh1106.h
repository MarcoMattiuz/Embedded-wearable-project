#include <stdint.h>
#include <driver/lcd_sh1106_driver.h>
#include "esp_lcd_types.h"
#include "driver/i2c_master.h"
#include "macros.h"

esp_lcd_panel_handle_t lcd_init(i2c_master_bus_handle_t *i2c_bus);
void drawCharToBuffer(char c, uint8_t *buffer_data, int x, int y);
void drawStringToBuffer(const char *str, uint8_t *buffer_data, int x, int y);
void drawBitmapToBuffer(const uint8_t *bitmap, uint8_t *buffer_data,
                        int x, int y, int width, int height);
void drawBufferToLcd(uint8_t *buffer_data, esp_lcd_panel_handle_t panel_handle);
void TurnLcdOn(esp_lcd_panel_handle_t panel_handle);
void TurnLcdOff(esp_lcd_panel_handle_t panel_handle);