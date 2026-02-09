# SH1106 ESP-IDF LCD Panel Driver

Driver for SH1106-based monochrome OLED displays.

- **Controller**: SH1106  
- **Bus**: I²C  
- **Color depth**: 1 bit per pixel (monochrome)  
- **Framework**: ESP-IDF  
- **License**: MIT  
- **Author**: TNY Robotics  
- **Version**: 1.0  
- **Repository**: https://github.com/tny-robotics/sh1106-esp-idf  

---

## Overview

This driver integrates the SH1106 OLED controller into the ESP-IDF `esp_lcd_panel` framework.  
It exposes a standard `esp_lcd_panel_t` interface.

Key features:
- I²C communication via `esp_lcd_panel_io`
- Hardware reset support (optional GPIO)
- Display initialization and power control
- Bitmap drawing
- Display mirroring (X/Y)
- Display ON/OFF control

Limitations:
- **No color inversion support** (hardware limitation)
- **No axis swapping**
- **Fixed resolution handling** (page-based, SH1106 specific)
---
## Memory mapping / Bitmap drawing

The SH1106 is a monochrome, page-addressed display, meaning that data is written in pages.
1 page = 8 vertical pixels and 128 horizontal pixels, the display is 128 x 64 -> 8 pages total.
We represent the screen buffer as an array of bytes (8 bits), the screen will read each byte as a page pixel (a vertical strip of 8 pixels high), and at the end of the page, it will move to the next page (8 pixels down).
Thus our buffer will be an array such as uint8_t buffer[128 * 8] (128 page pixels * 8 pages, with 8 pixels per page pixel).


```c

static esp_err_t panel_sh1106_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    sh1106_panel_t *sh1106 = __containerof(panel, sh1106_panel_t, base);
    esp_lcd_panel_io_handle_t io = sh1106->io;

    // For each line, shift at the line and send the bitmap line data
    I2C_LOCK_0();
    for (int y = 0; y < 8; y++) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, SH1106_CMD_SET_COLUMN_ADDR_LOW | 0x02, NULL, 0), TAG, "io tx param SH1106_CMD_SET_COLUMN_ADDR_LOW failed");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, SH1106_CMD_SET_COLUMN_ADDR_HIGH | 0x00, NULL, 0), TAG, "io tx param SH1106_CMD_SET_COLUMN_ADDR_HIGH failed");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, SH1106_CMD_SET_PAGE_ADDR | y, NULL, 0), TAG, "io tx param SH1106_CMD_SET_PAGE_ADDR failed");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, -1, color_data + y * SH1106_WIDTH, SH1106_WIDTH), TAG, "io tx color failed");
    }
    I2C_UNLOCK_0();

    return ESP_OK;
}
```

the function redraws the entire display. The loop iterates through the 8 pages; for each page, it sets the column start address and sends 128 bytes of data. Since its internal RAM is 132x64, SH1106 has a 2-pixel column offset  ( SH1106_CMD_SET_COLUMN_ADDR_LOW | 0x02), without it the image would shift.


