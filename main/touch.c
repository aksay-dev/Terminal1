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
#define GT911_REG_CONFIG_CHKSUM 0x80FF
#define GT911_REG_CONFIG_FRESH  0x8100

#define GT911_CONFIG_SIZE       (0xFF - 0x46) /* как в Arduino Touch_GT911 */

#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_FREQ_HZ  400000
#define I2C_MASTER_TIMEOUT  1000

/* Global touch coordinates (для совместимости старого API) */
int16_t touch_last_x = 0;
int16_t touch_last_y = 0;

/* Internal state */
static uint8_t gt911_addr = GT911_I2C_ADDR_28;
static bool initialized = false;
static uint8_t rotation = TOUCH_ROT_NORMAL;
static uint16_t panel_w = TOUCH_MAX_X;
static uint16_t panel_h = TOUCH_MAX_Y;
static uint8_t config_buf[GT911_CONFIG_SIZE] = {0};

/* Map function как в Arduino */
static int16_t map_value(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max)
{
    if (in_max == in_min) return out_min;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static void calc_checksum_and_mark_fresh(void)
{
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < GT911_CONFIG_SIZE; i++) {
        checksum += config_buf[i];
    }
    checksum = (uint8_t)((~checksum) + 1);
    config_buf[GT911_REG_CONFIG_CHKSUM - GT911_REG_CONFIG] = checksum;
    config_buf[GT911_REG_CONFIG_FRESH - GT911_REG_CONFIG] = 1;
}

static void apply_rotation(uint16_t *x, uint16_t *y)
{
    uint16_t x0 = *x;
    uint16_t y0 = *y;
    switch (rotation) {
        case TOUCH_ROT_LEFT:
            *x = panel_w - y0;
            *y = x0;
            break;
        case TOUCH_ROT_INVERTED:
            *x = panel_w - x0;
            *y = panel_h - y0;
            break;
        case TOUCH_ROT_RIGHT:
            *x = y0;
            *y = panel_h - x0;
            break;
        case TOUCH_ROT_NORMAL:
        default:
            *x = x0;
            *y = y0;
            break;
    }
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
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ GT911 not found on I2C bus");
        return false;
    }

    ESP_LOGI(TAG, "✓ GT911 detected at I2C addr 0x%02X", gt911_addr);
    ESP_LOGI(TAG, "✓ Product ID: %c%c%c%c", 
             product_id[0], product_id[1], product_id[2], product_id[3]);

    /* Считать текущий конфиг, чтобы можно было менять разрешение */
    ret = gt911_read_reg(GT911_REG_CONFIG, config_buf, GT911_CONFIG_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read config, continue without set_resolution (err=%s)", esp_err_to_name(ret));
    }

    initialized = true;
    return true;
}

bool touch_has_signal(void)
{
    return initialized;
}

void touch_set_rotation(uint8_t rot)
{
    rotation = rot;
}

bool touch_set_resolution(uint16_t width, uint16_t height)
{
    if (!initialized) return false;

    panel_w = width;
    panel_h = height;

    if (gt911_read_reg(GT911_REG_CONFIG, config_buf, GT911_CONFIG_SIZE) != ESP_OK) {
        ESP_LOGW(TAG, "read config failed");
        return false;
    }

    /* Обновить X/Y max */
    config_buf[GT911_REG_CONFIG + 0x01 - GT911_REG_CONFIG] = (uint8_t)(width & 0xFF);       /* X_OUTPUT_MAX_LOW  */
    config_buf[GT911_REG_CONFIG + 0x02 - GT911_REG_CONFIG] = (uint8_t)((width >> 8) & 0xFF);/* X_OUTPUT_MAX_HIGH */
    config_buf[GT911_REG_CONFIG + 0x03 - GT911_REG_CONFIG] = (uint8_t)(height & 0xFF);      /* Y_OUTPUT_MAX_LOW  */
    config_buf[GT911_REG_CONFIG + 0x04 - GT911_REG_CONFIG] = (uint8_t)((height >> 8) & 0xFF);/* Y_OUTPUT_MAX_HIGH */

    calc_checksum_and_mark_fresh();

    if (gt911_write_reg(GT911_REG_CONFIG, config_buf, GT911_CONFIG_SIZE) != ESP_OK) {
        ESP_LOGW(TAG, "write config failed");
        return false;
    }

    /* Подтвердить обновление */
    uint8_t fresh = 1;
    (void)gt911_write_reg(GT911_REG_CONFIG_FRESH, &fresh, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    return true;
}

bool touch_read_points(int16_t *xs, int16_t *ys, uint8_t *count)
{
    if (!initialized) {
        if (count) *count = 0;
        return false;
    }

    uint8_t status = 0;
    if (gt911_read_reg(GT911_REG_STATUS, &status, 1) != ESP_OK) {
        if (count) *count = 0;
        return false;
    }

    /* bit7 = buffer status, low nibble = points */
    if ((status & 0x80) == 0) {
        if (count) *count = 0;
        return false; /* нет новых данных */
    }

    uint8_t touches = status & 0x0F;
    if (touches == 0 || touches > 5) {
        uint8_t zero = 0;
        gt911_write_reg(GT911_REG_STATUS, &zero, 1);
        if (count) *count = 0;
        return false;
    }

    /* Читаем по 8 байт на точку */
    for (uint8_t i = 0; i < touches; i++) {
        uint8_t buf[8] = {0};
        if (gt911_read_reg(GT911_REG_DATA + i * 8, buf, 8) != ESP_OK) {
            touches = i;
            break;
        }
        uint16_t x = buf[1] | (buf[2] << 8);
        uint16_t y = buf[3] | (buf[4] << 8);
        apply_rotation(&x, &y);
        if (xs) xs[i] = (int16_t)x;
        if (ys) ys[i] = (int16_t)y;
    }

    /* сбрасываем флаг готовности */
    uint8_t zero = 0;
    gt911_write_reg(GT911_REG_STATUS, &zero, 1);

    if (count) *count = touches;
    return touches > 0;
}

bool touch_touched(void)
{
    int16_t xs[5], ys[5];
    uint8_t cnt = 0;
    bool ok = touch_read_points(xs, ys, &cnt);
    if (ok && cnt > 0) {
        /* Совместимость: берём первую точку */
        touch_last_x = xs[0];
        touch_last_y = ys[0];
        return true;
    }
    return false;
}

bool touch_released(void)
{
    /* GT911 в простой реализации всегда возвращает true когда нет касания */
    return initialized;
}
