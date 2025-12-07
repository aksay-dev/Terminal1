/**
 * @file touch.c
 * @brief GT911 Touchscreen driver для ESP-IDF
 * 
 * Портировано из Arduino проекта 4.0_LvglWidgets
 * Использует ту же логику что и TAMC_GT911 библиотека
 */

#include "touch.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "GT911";

/* GT911 I2C addresses */
#define GT911_I2C_ADDR_28   0x5D  /* Default address */
#define GT911_I2C_ADDR_BA   0x14  /* Alternative address */

/* GT911 Registers */
#define GT911_REG_PRODUCT_ID    0x8140
#define GT911_REG_CONFIG        0x8047
#define GT911_REG_STATUS        0x814E
#define GT911_REG_DATA          0x814F

#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_FREQ_HZ  400000
#define I2C_MASTER_TIMEOUT  1000

/* Global touch coordinates */
int16_t touch_last_x = 0;
int16_t touch_last_y = 0;

/* Internal state */
static uint8_t gt911_addr = GT911_I2C_ADDR_28;
static bool initialized = false;

/* Map function как в Arduino */
static int16_t map_value(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max)
{
    if (in_max == in_min) return out_min;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * @brief Write data to GT911 register
 */
static esp_err_t gt911_write_reg(uint16_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (gt911_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg >> 8, true);
    i2c_master_write_byte(cmd, reg & 0xFF, true);
    if (len > 0) {
        i2c_master_write(cmd, data, len, true);
    }
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief Read data from GT911 register
 */
static esp_err_t gt911_read_reg(uint16_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (gt911_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg >> 8, true);
    i2c_master_write_byte(cmd, reg & 0xFF, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (gt911_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT));
    i2c_cmd_link_delete(cmd);
    return ret;
}

bool touch_init(void)
{
    ESP_LOGI(TAG, "Initializing GT911 touchscreen...");
    
    /* Configure I2C */
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = TOUCH_GT911_SDA,
        .scl_io_num = TOUCH_GT911_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C config failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return false;
    }

    /* Small delay for GT911 to stabilize */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Try to read product ID */
    uint8_t product_id[4] = {0};
    ret = gt911_read_reg(GT911_REG_PRODUCT_ID, product_id, 4);
    
    if (ret != ESP_OK) {
        /* Try alternative address */
        ESP_LOGW(TAG, "Trying alternative I2C address...");
        gt911_addr = GT911_I2C_ADDR_BA;
        vTaskDelay(pdMS_TO_TICKS(50));
        ret = gt911_read_reg(GT911_REG_PRODUCT_ID, product_id, 4);
    }
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ GT911 detected at I2C addr 0x%02X", gt911_addr);
        ESP_LOGI(TAG, "✓ Product ID: %c%c%c%c", 
                 product_id[0], product_id[1], product_id[2], product_id[3]);
        initialized = true;
        return true;
    } else {
        ESP_LOGE(TAG, "✗ GT911 not found on I2C bus");
        return false;
    }
}

bool touch_has_signal(void)
{
    return initialized;
}

bool touch_touched(void)
{
    if (!initialized) {
        return false;
    }

    /* Read touch status */
    uint8_t status = 0;
    esp_err_t ret = gt911_read_reg(GT911_REG_STATUS, &status, 1);
    
    if (ret != ESP_OK) {
        return false;
    }
    
    /* Check if data is ready (bit 7) */
    if ((status & 0x80) == 0) {
        return false;  /* No new data */
    }

    /* Get number of touch points */
    uint8_t touch_points = status & 0x0F;
    
    /* Clear status register first */
    uint8_t zero = 0;
    gt911_write_reg(GT911_REG_STATUS, &zero, 1);
    
    if (touch_points == 0 || touch_points > 5) {
        return false;
    }

    /* Read first touch point data (8 bytes: track_id, xl, xh, yl, yh, size_l, size_h, reserved) */
    uint8_t data[8] = {0};
    ret = gt911_read_reg(GT911_REG_DATA, data, 8);
    
    if (ret != ESP_OK) {
        return false;
    }
    
    /* Extract coordinates - GT911 uses little-endian */
    uint16_t x_raw = data[1] | (data[2] << 8);  /* xl, xh */
    uint16_t y_raw = data[3] | (data[4] << 8);  /* yl, yh */
    
    /* Apply coordinate mapping как в Arduino проекте */
    /* TOUCH_MAP_X1=480, TOUCH_MAP_X2=0 означает инверсия X */
    /* TOUCH_MAP_Y1=480, TOUCH_MAP_Y2=0 означает инверсия Y */
    touch_last_x = map_value(x_raw, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, TOUCH_MAX_X - 1);
    touch_last_y = map_value(y_raw, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, TOUCH_MAX_Y - 1);
    
    return true;
}

bool touch_released(void)
{
    /* GT911 в простой реализации всегда возвращает true когда нет касания */
    return initialized;
}
