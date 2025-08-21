// lcd_i2c.h

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_MASTER_SDA_IO           GPIO_NUM_21
#define I2C_MASTER_SCL_IO           GPIO_NUM_22
#define I2C_MASTER_NUM              I2C_NUM_0

#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0

#define LCD_I2C_ADDR                0x27
#define LCD_COLS                    16
#define LCD_ROWS                    2

#define PCF8574_RS                  0b00000001
#define PCF8574_RW                  0b00000010
#define PCF8574_EN                  0b00000100
#define PCF8574_BL                  0b00001000

#define PCF8574_D4                  0b00010000
#define PCF8574_D5                  0b00100000
#define PCF8574_D6                  0b01000000
#define PCF8574_D7                  0b10000000

#define LCD_RS_COMMAND              0b00000000
#define LCD_RS_DATA                 0b00000001

#define LCD_CLEARDISPLAY            0b00000001
#define LCD_RETURNHOME              0b00000010
#define LCD_ENTRYMODESET            0b00000100
#define LCD_DISPLAYMODECONTROL      0b00001000
#define LCD_CURSORSHIFT             0b00010000
#define LCD_FUNCTIONSET             0b00100000
#define LCD_SETCGRAMADDR            0b01000000
#define LCD_SETDDRAMADDR            0b10000000

#define LCD_DISPLAYON               0b00000100
#define LCD_CURSOROFF               0b00000000
#define LCD_CURSORON                0b00000010
#define LCD_BLINKOFF                0b00000000
#define LCD_BLINKON                 0b00000001  

#define LCD_ENTRYSHIFTINCREMENT     0b00000010
#define LCD_ENTRYSHIFTLEFTON        0b00000000 
#define LCD_ENTRYSHIFTLEFTOFF       0b00000001

#define LCD_8BITMODE                0b00010000
#define LCD_4BITMODE                0b00000000
#define LCD_2LINE                   0b00001000
#define LCD_1LINE                   0b00000000
#define LCD_5x10DOTS                0b00000100
#define LCD_5x8DOTS                 0b00000000

typedef struct {
    i2c_master_dev_handle_t i2c_dev_handle;
    uint8_t cols;
    uint8_t rows;
    uint8_t backlight_state;
} lcd_i2c_handle_t;

lcd_i2c_handle_t* lcd_i2c_init(void);

void lcd_i2c_backlight(lcd_i2c_handle_t* lcd, bool on);
esp_err_t lcd_i2c_home(lcd_i2c_handle_t* lcd);
esp_err_t lcd_i2c_clear(lcd_i2c_handle_t* lcd);
esp_err_t lcd_i2c_set_cursor(lcd_i2c_handle_t* lcd, uint8_t col, uint8_t row);
esp_err_t lcd_i2c_write_char(lcd_i2c_handle_t* lcd, char c);
esp_err_t lcd_i2c_write_string(lcd_i2c_handle_t* lcd, const char* str, ...);

#ifdef __cplusplus
}
#endif