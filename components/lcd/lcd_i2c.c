// lcd_i2c.c

#include "lcd_i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"
#include <stdarg.h>

static const char* TAG = "LCD_I2C_DRIVER";

static i2c_master_bus_handle_t _lcd_i2c_master_init(void) {
    i2c_master_bus_config_t i2c_conf = {
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .i2c_port                     = I2C_MASTER_NUM,
        .scl_io_num                   = I2C_MASTER_SCL_IO,
        .sda_io_num                   = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt            = 7,
        .flags.enable_internal_pullup = false,
    };

    i2c_master_bus_handle_t i2c_bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_conf, &i2c_bus_handle));
    return i2c_bus_handle;
}

static esp_err_t _lcd_send_byte_i2c(lcd_i2c_handle_t* lcd, uint8_t val) {
    uint8_t write_buffer[1] = {val};
    ESP_ERROR_CHECK(i2c_master_transmit(
        lcd->i2c_dev_handle,
        write_buffer,
        sizeof(write_buffer),
        pdMS_TO_TICKS(1000)));

    return ESP_OK;
}

static lcd_i2c_handle_t* _lcd_i2c_create(i2c_master_bus_handle_t i2c_bus_handle, uint8_t address, uint8_t cols, uint8_t rows) {
    if (i2c_bus_handle == NULL) {
        ESP_LOGE(TAG, "I2C BUS HANDLE IS NULL, CANNOT CREATE LCD");
        return NULL;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = address,
        .scl_speed_hz    = I2C_MASTER_FREQ_HZ};

    i2c_master_dev_handle_t i2c_dev_handle = NULL;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &i2c_dev_handle));

    lcd_i2c_handle_t* lcd = (lcd_i2c_handle_t*)calloc(1, sizeof(lcd_i2c_handle_t));
    if (lcd == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for LCD handle!");
        i2c_master_bus_rm_device(i2c_dev_handle);
        return NULL;
    }

    lcd->i2c_dev_handle  = i2c_dev_handle;
    lcd->cols            = cols;
    lcd->rows            = rows;
    lcd->backlight_state = 0;

    return lcd;
}

static esp_err_t _lcd_write_4bit_nibble(lcd_i2c_handle_t* lcd, uint8_t nibble, uint8_t mode) {
    uint8_t data_to_send;

    data_to_send = mode | lcd->backlight_state;

    if (nibble & 0b0001) {
        data_to_send |= PCF8574_D4;
    }

    if ((nibble >> 1) & 0b0001) {
        data_to_send |= PCF8574_D5;
    }

    if ((nibble >> 2) & 0b0001) {
        data_to_send |= PCF8574_D6;
    }

    if ((nibble >> 3) & 0b0001) {
        data_to_send |= PCF8574_D7;
    }

    data_to_send |= PCF8574_EN;
    ESP_ERROR_CHECK(_lcd_send_byte_i2c(lcd, data_to_send));
    esp_rom_delay_us(1);

    data_to_send &= ~PCF8574_EN;
    ESP_ERROR_CHECK(_lcd_send_byte_i2c(lcd, data_to_send));
    esp_rom_delay_us(50);

    return ESP_OK;
}

static esp_err_t _lcd_send_cmd(lcd_i2c_handle_t* lcd, uint8_t cmd) {
    ESP_ERROR_CHECK(_lcd_write_4bit_nibble(lcd, (cmd >> 4) & 0x0F, LCD_RS_COMMAND));
    ESP_ERROR_CHECK(_lcd_write_4bit_nibble(lcd, cmd & 0x0F, LCD_RS_COMMAND));

    if (cmd == LCD_CLEARDISPLAY || cmd == LCD_RETURNHOME) {
        esp_rom_delay_us(30000);
    } else {
        esp_rom_delay_us(300);
    }

    return ESP_OK;
}

static esp_err_t _lcd_send_data(lcd_i2c_handle_t* lcd, uint8_t data) {
    ESP_ERROR_CHECK(_lcd_write_4bit_nibble(lcd, (data >> 4) & 0x0F, LCD_RS_DATA));
    ESP_ERROR_CHECK(_lcd_write_4bit_nibble(lcd, data & 0x0F, LCD_RS_DATA));
    esp_rom_delay_us(50);

    return ESP_OK;
}

static void _lcd_init(lcd_i2c_handle_t* lcd) {
    ESP_LOGI(TAG, "INITIALIZING LCD DISPLAY SEQUENCE");
    esp_rom_delay_us(50000);

    _lcd_write_4bit_nibble(lcd, 0x03, LCD_RS_COMMAND);
    esp_rom_delay_us(2000);

    _lcd_write_4bit_nibble(lcd, 0x03, LCD_RS_COMMAND);
    esp_rom_delay_us(150);

    _lcd_write_4bit_nibble(lcd, 0x03, LCD_RS_COMMAND);
    esp_rom_delay_us(150);

    _lcd_write_4bit_nibble(lcd, 0x02, LCD_RS_COMMAND);
    esp_rom_delay_us(100);

    _lcd_send_cmd(lcd, LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS);
    _lcd_send_cmd(lcd, LCD_DISPLAYMODECONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);
    _lcd_send_cmd(lcd, LCD_CLEARDISPLAY);

    _lcd_send_cmd(lcd, LCD_ENTRYMODESET | LCD_ENTRYSHIFTINCREMENT);
    _lcd_send_cmd(lcd, LCD_RETURNHOME);

    lcd_i2c_backlight(lcd, true);
    ESP_LOGI(TAG, "LCD INITIALIZING SEQUENCE COMPLETE");
}

void lcd_i2c_backlight(lcd_i2c_handle_t* lcd, bool on) {
    uint8_t data_to_send = 0;

    if (on) {
        lcd->backlight_state = PCF8574_BL;
        data_to_send         = PCF8574_BL;
        ESP_LOGI(TAG, "LCD Backlight ON");
    } else {
        lcd->backlight_state = 0;
        ESP_LOGI(TAG, "LCD Backlight OFF");
    }
    ESP_ERROR_CHECK(_lcd_send_byte_i2c(lcd, data_to_send));
}

esp_err_t lcd_i2c_clear(lcd_i2c_handle_t* lcd) {
    if (lcd == NULL) {
        ESP_LOGE(TAG, "LCD handle is NULL in lcd_i2c_clear");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGD(TAG, "Clearing LCD display");
    return _lcd_send_cmd(lcd, LCD_CLEARDISPLAY);
}

esp_err_t lcd_i2c_home(lcd_i2c_handle_t* lcd) {
    if (lcd == NULL) {
        ESP_LOGE(TAG, "LCD handle is NULL in lcd_i2c_home");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGD(TAG, "Returning Cursor to Home");

    return _lcd_send_cmd(lcd, LCD_RETURNHOME);
}

esp_err_t lcd_i2c_set_cursor(lcd_i2c_handle_t* lcd, uint8_t col, uint8_t row) {
    if (lcd == NULL) {
        ESP_LOGE(TAG, "LCD handle is NULL in lcd_i2c_set_cursor");
        return ESP_ERR_INVALID_ARG;
    }

    if (col >= lcd->cols) {
        ESP_LOGW(TAG, "Column %d out of bounds (max %d). Clamping.", col, lcd->cols - 1);
        col = lcd->cols - 1;
    }

    if (row >= lcd->rows) {
        ESP_LOGW(TAG, "Row %d out of bounds (max %d). Clamping.", row, lcd->rows - 1);
        row = lcd->rows - 1;
    }

    uint8_t address = 0;
    if (row == 0) {
        address = 0x0;
    } else if (row == 1) {
        address = 0x40;
    } else {
        ESP_LOGE(TAG, "Invalid row %d for LCD type", row);
        return ESP_ERR_INVALID_ARG;
    }

    address += col;

    ESP_LOGD(TAG, "Setting cursor to col %d, row %d (DDRAM address 0x%02x)", col, row, address);

    return _lcd_send_cmd(lcd, LCD_SETDDRAMADDR | address);
}

esp_err_t lcd_i2c_write_char(lcd_i2c_handle_t* lcd, char c) {
    if (lcd == NULL) {
        ESP_LOGE(TAG, "LCD handle is NULL in lcd_i2c_write_char");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGD(TAG, "Printing character '%c' (0x%02x)", c, c);

    return _lcd_send_data(lcd, (uint8_t)c);
}

esp_err_t lcd_i2c_write_string(lcd_i2c_handle_t* lcd, const char* str, ...) {
    if (lcd == NULL) {
        ESP_LOGE(TAG, "LCD handle is NULL in lcd_i2c_write_string");
        return ESP_ERR_INVALID_ARG;
    }

    if (str == NULL) {
        ESP_LOGW(TAG, "Attempted to print a NULL string.");
        return ESP_ERR_INVALID_ARG;
    }

    char print_buffer[40];

    va_list args;
    va_start(args, str);

    int chars_written = vsnprintf(print_buffer, sizeof(print_buffer), str, args);

    va_end(args);

    if (chars_written < 0) {
        ESP_LOGE(TAG, "Error formatting string for LCD: %d", chars_written);
        return ESP_FAIL;
    }

    esp_err_t ret = ESP_OK;
    ESP_LOGD(TAG, "Printing formatted string: \"%s\"", print_buffer);

    int strIndex = 0;

    while (print_buffer[strIndex] != '\0' && strIndex < lcd->cols) {
        ret = lcd_i2c_write_char(lcd, print_buffer[strIndex]);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to print character '%c' (0x%02x) from formatted string at index %d",
                     print_buffer[strIndex], print_buffer[strIndex], strIndex);
            return ret;
        }
        strIndex++;
    }

    return ESP_OK;
}

lcd_i2c_handle_t* lcd_i2c_init(void) {
    ESP_LOGI(TAG, "Initializing LCD");
    i2c_master_bus_handle_t i2c_bus = _lcd_i2c_master_init();
    if (i2c_bus == NULL) {
        ESP_LOGE(TAG, "FAILED TO INITIALIZE I2C BUS");
        return NULL;
    }
    ESP_LOGI(TAG, "INITIALIZED I2C BUS");

    lcd_i2c_handle_t* lcd_handle = _lcd_i2c_create(i2c_bus, LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
    if (lcd_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create LCD handle!");
        return NULL;
    }
    ESP_LOGI(TAG, "LCD handle created");
    _lcd_init(lcd_handle);

    return lcd_handle;
}