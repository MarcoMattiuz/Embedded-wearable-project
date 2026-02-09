# lcd_sh1106.md
Using the sh1106 driver, it implements:
- A buffer mapped to SH1106 memory (see lcd_sh1106_driver.md for details)
- String rendering using a 5×7 font
- Bitmap rendering
- LCD initialization
  
### esp_lcd_panel_handle_t lcd_init(i2c_master_bus_handle_t *i2c_bus)
Initializes the SH1106 display over I2C and returns a panel handle.
### void drawBufferToLcd(uint8_t *buffer_data, esp_lcd_panel_handle_t panel_handle)
Transfers the entire buffer to the display.
This function does not modify the buffer, it only sends it to the display.
### TurnLcdOn / TurnLcdOff
Turns display screen on/off.
### void drawCharToBuffer(char c, uint8_t *buffer_data, int x, int y)
Draws a single ASCII character, using the 5x7 font, into the buffer starting from position (x,y).
### void drawStringToBuffer(const char *str, uint8_t *buffer_data, int x, int y);
Draws a string, using the 5x7 font, into the buffer starting from position (x,y).
### void drawBitmapToBuffer(const uint8_t *bitmap,uint8_t *buffer_data,int x,int y,int width, int height)
Draws a monochrome bitmap into the buffer starting from position (x,y).
