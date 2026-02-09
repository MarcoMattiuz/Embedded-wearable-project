
/*
uint_8 buffer[128*8] is mapped to the display
8 pages, one page is 128 page pixels
one page pixel is a column of 8 bit/pixels
bit 0 of page 0 is top leftmost pixel
*/
#include "lcd_sh1106.h"

#include "freertos/FreeRTOS.h"
#include "driver/i2c_master.h"
#include "esp_lcd_io_i2c.h"
#include "driver/lcd_sh1106_driver.h"
#include "I2C_api.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#include <string.h>
#include "font5x7.h"
#include "bitmaps.h"


// map ascii charachter in 5*7 font to the buffer at x,y
void drawCharToBuffer(char c, uint8_t *buffer_data, int x, int y)
{
    // check screen limit
    if (x < 0 || x > SH1106_WIDTH - 5 || y < 0 || y > SH1106_HEIGHT - 7)
        return;

    for (int col = 0; col < 5; col++)
    {
        // line is a byte representing column of 8 bits/pixels (a page pixel)
        uint8_t line = font5x7[(uint8_t)c][col];

        // loop over each bit in the column (row)
        for (int row = 0; row < 8; row++)
        {
            int pixelY = y + row;
            int page = pixelY / 8; // page index of the pixel
            int bit = pixelY % 8;  // bit shift of the pixel in the page pixel

            if (line & (1 << row))
            {
                buffer_data[page * SH1106_WIDTH + x + col] |= (1 << bit); // turn on
            }
            else
            {
                buffer_data[page * SH1106_WIDTH + x + col] &= ~(1 << bit); // turn off
            }
        }
    }
}

void drawStringToBuffer(const char *str, uint8_t *buffer_data, int x, int y)
{
    int startX = x; // remember starting X for newlines

    while (*str)
    {
        if (*str == '\n')
        {
            // Move to next line
            y += 8;     // line height (7 px font + 1 px spacing)
            x = startX; // reset x position
        }
        else
        {
            // Draw character and move cursor right
            drawCharToBuffer(*str, buffer_data, x, y);
            x += 6; // 5 px char width + 1 px spacing

            // if exceeding display width
            if (x > SH1106_WIDTH - 6)
            {
                y += 8;
                x = startX;
            }
        }
        str++;
    }
}

void drawBitmapToBuffer(const uint8_t *bitmap, uint8_t *buffer_data,
                        int x, int y, int width, int height)
{
    // loop through all pixels of the bitmap
    for (int j = 0; j < height; j++) // rows
    {
        for (int i = 0; i < width; i++) // columns
        {
            // find which byte and bit contain this pixel in the bitmap
            int byteIndex = (j * width + i) / 8;
            int bitIndex = 7 - ((j * width + i) % 8); // 7- ensures pixels are read MSB first

            // extract the bit (1 = ON, 0 = OFF)
            // shifts the desired bit 0 position
            // + masks off everything except that single bit.
            uint8_t bit = (bitmap[byteIndex] >> bitIndex) & 0x01;

            //translate to display coordinates
            int pixelX = x + i;
            int pixelY = y + j;

            //check boundaries
            if (pixelX < 0 || pixelX >= SH1106_WIDTH || pixelY < 0 || pixelY >= SH1106_HEIGHT)
                continue;

            
            int page = pixelY / 8;
            int bitPos = pixelY % 8;

            if (bit)
                buffer_data[page * SH1106_WIDTH + pixelX] |= (1 << bitPos);
            else
                buffer_data[page * SH1106_WIDTH + pixelX] &= ~(1 << bitPos);
        }
    }
}

esp_lcd_panel_handle_t lcd_init(i2c_master_bus_handle_t *i2c_bus)
{
    /* I2C CONFIGURATION */

    // i2c bus configuration
    
    /*
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_HOST,              // I2C port number
        .sda_io_num = I2C_SDA_GPIO,        // GPIO number for I2C sda signal
        .scl_io_num = I2C_SCL_GPIO,        // GPIO number for I2C scl signal
        .clk_source = I2C_CLK_SRC_DEFAULT, // I2C clock source, just use the default
        .glitch_ignore_cnt = 7,            // glitch filter, again, just use the default
        .intr_priority = 0,                // interrupt priority, default to 0
        .trans_queue_depth = 0,            // transaction queue depth, default to 0
        .flags = {
            .enable_internal_pullup = true, // enable internal pullup resistors (oled screen does not have one)
            .allow_pd = false,              // just using the default value
        },
    };
    */
    
    // Create the i2c io handle
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = ESP_SH1106_DEFAULT_IO_CONFIG;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(*i2c_bus, &io_config, &io_handle));

    /* SCREEN CONFIGURATION */

    // sh1106 panel configuration (most of the values are not used, but must be set to avoid cpp warnings)
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,                         // sh1106 does not have a reset pin, so set to -1
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,   // not even used, but must be set to avoid cpp warnings
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,    // not even used, but must be set to avoid cpp warnings
        .bits_per_pixel = SH1106_PIXELS_PER_BYTE / 8, // bpp = 1 (monochrome, that's important)
        .flags = {
            .reset_active_high = false, // not even used, but must be set to avoid cpp warnings
        },
        .vendor_config = NULL, // no need for custom vendor config, not implemented
    };

    // Create the panel handle from the sh1106 driver
    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh1106(io_handle, &panel_config, &panel_handle));

    // Initialize the screen
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    return panel_handle;
}
//send buffer to display
void drawBufferToLcd(uint8_t *buffer_data, esp_lcd_panel_handle_t panel_handle)
{
    esp_lcd_panel_draw_bitmap(
        panel_handle,
        0, 0,
        SH1106_WIDTH, SH1106_HEIGHT,
        buffer_data);
}
void TurnLcdOn(esp_lcd_panel_handle_t panel_handle){
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
}
void TurnLcdOff(esp_lcd_panel_handle_t panel_handle)
{
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
}