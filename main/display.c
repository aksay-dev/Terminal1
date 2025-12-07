#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "rom/ets_sys.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_io.h"
#include "display.h"

/* Пины из Arduino-проекта */
#define LCD_DE_GPIO          18
#define LCD_VSYNC_GPIO       17
#define LCD_HSYNC_GPIO       16
#define LCD_PCLK_GPIO        21

#define LCD_R0_GPIO          11
#define LCD_R1_GPIO          12
#define LCD_R2_GPIO          13
#define LCD_R3_GPIO          14
#define LCD_R4_GPIO          0

#define LCD_G0_GPIO          8
#define LCD_G1_GPIO          20
#define LCD_G2_GPIO          3
#define LCD_G3_GPIO          46
#define LCD_G4_GPIO          9
#define LCD_G5_GPIO          10

#define LCD_B0_GPIO          4
#define LCD_B1_GPIO          5
#define LCD_B2_GPIO          6
#define LCD_B3_GPIO          7
#define LCD_B4_GPIO          15

#define LCD_BL_GPIO          38

/* 3-wire SPI to ST7701 (from Arduino project) */
#define LCD_CMD_CS_GPIO      39
#define LCD_CMD_SCL_GPIO     48
#define LCD_CMD_SDA_GPIO     47

static inline void st7701_gpio_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << LCD_CMD_CS_GPIO) | (1ULL << LCD_CMD_SCL_GPIO) | (1ULL << LCD_CMD_SDA_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(LCD_CMD_CS_GPIO, 1);
    gpio_set_level(LCD_CMD_SCL_GPIO, 1);
    gpio_set_level(LCD_CMD_SDA_GPIO, 1);
}

static inline void st7701_clk_pulse(void)
{
    gpio_set_level(LCD_CMD_SCL_GPIO, 0);
    ets_delay_us(1);
    gpio_set_level(LCD_CMD_SCL_GPIO, 1);
    ets_delay_us(1);
}

/* Send 9-bit frame: first bit indicates cmd/data (0=cmd,1=data), then 8-bit payload */
static void st7701_write9(uint8_t dc_is_data, uint8_t payload)
{
    gpio_set_level(LCD_CMD_CS_GPIO, 0);
    gpio_set_level(LCD_CMD_SDA_GPIO, dc_is_data ? 1 : 0);
    st7701_clk_pulse();
    for (int i = 7; i >= 0; --i) {
        gpio_set_level(LCD_CMD_SDA_GPIO, (payload >> i) & 1);
        st7701_clk_pulse();
    }
    gpio_set_level(LCD_CMD_CS_GPIO, 1);
    ets_delay_us(2);
}

static inline void st7701_cmd(uint8_t cmd) { st7701_write9(0, cmd); }
static inline void st7701_data(uint8_t data) { st7701_write9(1, data); }

/* Minimal ST7701 init (based on Arduino st7701_type1_init_operations) */
static void st7701_init_sequence(void)
{
    st7701_gpio_init();

    // Page 0
    st7701_cmd(0xFF); st7701_data(0x77); st7701_data(0x01); st7701_data(0x00); st7701_data(0x00); st7701_data(0x10);
    st7701_cmd(0xC0); st7701_data(0x3B); st7701_data(0x00);
    st7701_cmd(0xC1); st7701_data(0x0D); st7701_data(0x02);
    st7701_cmd(0xC2); st7701_data(0x31); st7701_data(0x05);
    st7701_cmd(0xCD); st7701_data(0x00);

    // Gamma
    st7701_cmd(0xB0);
    uint8_t b0[] = {0x00,0x11,0x18,0x0E,0x11,0x06,0x07,0x08,0x07,0x22,0x04,0x12,0x0F,0xAA,0x31,0x18};
    for (int i=0;i<16;i++) st7701_data(b0[i]);
    st7701_cmd(0xB1);
    uint8_t b1[] = {0x00,0x11,0x19,0x0E,0x12,0x07,0x08,0x08,0x08,0x22,0x04,0x11,0x11,0xA9,0x32,0x18};
    for (int i=0;i<16;i++) st7701_data(b1[i]);

    // Page 1
    st7701_cmd(0xFF); st7701_data(0x77); st7701_data(0x01); st7701_data(0x00); st7701_data(0x00); st7701_data(0x11);
    st7701_cmd(0xB0); st7701_data(0x60);
    st7701_cmd(0xB1); st7701_data(0x32);
    st7701_cmd(0xB2); st7701_data(0x07);
    st7701_cmd(0xB3); st7701_data(0x80);
    st7701_cmd(0xB5); st7701_data(0x49);
    st7701_cmd(0xB7); st7701_data(0x85);
    st7701_cmd(0xB8); st7701_data(0x21);
    st7701_cmd(0xC1); st7701_data(0x78);
    st7701_cmd(0xC2); st7701_data(0x78);

    st7701_cmd(0xE0); st7701_data(0x00); st7701_data(0x1B); st7701_data(0x02);
    st7701_cmd(0xE1); uint8_t e1[] = {0x08,0xA0,0x00,0x00,0x07,0xA0,0x00,0x00,0x00,0x44,0x44};
    for (int i=0;i<11;i++) st7701_data(e1[i]);
    st7701_cmd(0xE2); uint8_t e2[] = {0x11,0x11,0x44,0x44,0xED,0xA0,0x00,0x00,0xEC,0xA0,0x00,0x00};
    for (int i=0;i<12;i++) st7701_data(e2[i]);
    st7701_cmd(0xE3); st7701_data(0x00); st7701_data(0x00); st7701_data(0x11); st7701_data(0x11);
    st7701_cmd(0xE4); st7701_data(0x44); st7701_data(0x44);
    st7701_cmd(0xE5); uint8_t e5[] = {0x0A,0xE9,0xD8,0xA0,0x0C,0xEB,0xD8,0xA0,0x0E,0xED,0xD8,0xA0,0x10,0xEF,0xD8,0xA0};
    for (int i=0;i<16;i++) st7701_data(e5[i]);
    st7701_cmd(0xE6); st7701_data(0x00); st7701_data(0x00); st7701_data(0x11); st7701_data(0x11);
    st7701_cmd(0xE7); st7701_data(0x44); st7701_data(0x44);
    st7701_cmd(0xE8); uint8_t e8[] = {0x09,0xE8,0xD8,0xA0,0x0B,0xEA,0xD8,0xA0,0x0D,0xEC,0xD8,0xA0,0x0F,0xEE,0xD8,0xA0};
    for (int i=0;i<16;i++) st7701_data(e8[i]);
    st7701_cmd(0xEB); uint8_t eb[] = {0x02,0x00,0xE4,0xE4,0x88,0x00,0x40};
    for (int i=0;i<7;i++) st7701_data(eb[i]);
    st7701_cmd(0xEC); st7701_data(0x3C); st7701_data(0x00);

    // VAP/VAN
    st7701_cmd(0xFF); st7701_data(0x77); st7701_data(0x01); st7701_data(0x00); st7701_data(0x00); st7701_data(0x13);
    st7701_cmd(0xE5); st7701_data(0xE4);

    // Back to page 0 and set pixel format RGB565 (без изменения порядка RGB/BGR)
    st7701_cmd(0xFF); st7701_data(0x77); st7701_data(0x01); st7701_data(0x00); st7701_data(0x00); st7701_data(0x00);
    st7701_cmd(0x3A); st7701_data(0x50);
    
    // MADCTL: попробуем разные варианты
    // st7701_cmd(0x36); st7701_data(0x08); // BGR
    // st7701_cmd(0x36); st7701_data(0x00); // RGB (по умолчанию)

    ets_delay_us(10*1000);
    st7701_cmd(0x11); // Sleep Out
    ets_delay_us(120*1000);
    st7701_cmd(0x29); // Display On
    ets_delay_us(20*1000);
}

#define LCD_H_RES            480
#define LCD_V_RES            480

/* Тайминги из скетча - увеличены порчи */
#define HSYNC_FRONT_PORCH    10
#define HSYNC_PULSE_WIDTH    8
#define HSYNC_BACK_PORCH     50
#define VSYNC_FRONT_PORCH    8
#define VSYNC_PULSE_WIDTH    8
#define VSYNC_BACK_PORCH     22
#define PCLK_HZ              (10 * 1000 * 1000)

/* LVGL таймер период (мс) */
#define LVGL_TICK_MS         5

static lv_display_t *s_lv_display = NULL;
static esp_lcd_panel_handle_t s_rgb_panel = NULL;
static esp_timer_handle_t s_lvgl_tick_timer = NULL;
static bool s_bl_inited = false;

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_TICK_MS);
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2 + 1;
    int y2 = area->y2 + 1;
    esp_lcd_panel_draw_bitmap(s_rgb_panel, x1, y1, x2, y2, px_map);
    lv_display_flush_ready(disp);
}

void display_init(void)
{
    /* Подсветка: постоянный HIGH на GPIO */
    gpio_config_t bl_io = {
        .pin_bit_mask = 1ULL << LCD_BL_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&bl_io));
    gpio_set_level(LCD_BL_GPIO, 1);
    s_bl_inited = true;

    /* Инициализация контроллера ST7701 (командный интерфейс 3-wire SPI) */
    vTaskDelay(pdMS_TO_TICKS(500));
    st7701_init_sequence();
    vTaskDelay(pdMS_TO_TICKS(500));
    

    /* Конфигурация RGB панели */
    esp_lcd_rgb_panel_config_t rgb_config = {
        .data_width = 16, /* R5G6B5 через 5+6+5 линий, фактически 5:6:5 на 16 линий */
        .bits_per_pixel = 16,
        .num_fbs = 0, /* не выделять внутренние фрейм-буферы */
        .bounce_buffer_size_px = 0, /* отключить bounce-буфер внутри драйвера */
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .hsync_gpio_num = LCD_HSYNC_GPIO,
        .vsync_gpio_num = LCD_VSYNC_GPIO,
        .de_gpio_num = LCD_DE_GPIO,
        .pclk_gpio_num = LCD_PCLK_GPIO,
        .disp_gpio_num = -1,
        .timings = {
            .pclk_hz = PCLK_HZ,
            .h_res = LCD_H_RES,
            .v_res = LCD_V_RES,
            .hsync_front_porch = HSYNC_FRONT_PORCH,
            .hsync_pulse_width = HSYNC_PULSE_WIDTH,
            .hsync_back_porch = HSYNC_BACK_PORCH,
            .vsync_front_porch = VSYNC_FRONT_PORCH,
            .vsync_pulse_width = VSYNC_PULSE_WIDTH,
            .vsync_back_porch = VSYNC_BACK_PORCH,
            .flags = {
                .pclk_active_neg = false,
                .hsync_idle_low = false,   /* Попробуем инвертировать HSYNC */
                .vsync_idle_low = false,   /* И VSYNC тоже */
                .de_idle_high = false,     /* И DE тоже */
            },
        },
        .flags = {
            .fb_in_psram = true,   // AAA
            .disp_active_low = false,
        },
        .data_gpio_nums = {
            LCD_B0_GPIO, /* D0  */
            LCD_B1_GPIO, /* D1  */
            LCD_B2_GPIO, /* D2  */
            LCD_B3_GPIO, /* D3  */
            LCD_B4_GPIO, /* D4  */
            LCD_G0_GPIO, /* D5  */
            LCD_G1_GPIO, /* D6  */
            LCD_G2_GPIO, /* D7  */
            LCD_G3_GPIO, /* D8  */
            LCD_G4_GPIO, /* D9  */
            LCD_G5_GPIO, /* D10 */
            LCD_R0_GPIO, /* D11 */
            LCD_R1_GPIO, /* D12 */
            LCD_R2_GPIO, /* D13 */
            LCD_R3_GPIO, /* D14 */
            LCD_R4_GPIO, /* D15 */
        },
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&rgb_config, &s_rgb_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_rgb_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_rgb_panel));
    /* На всякий случай включим отображение */
    (void)esp_lcd_panel_disp_on_off(s_rgb_panel, true);

    /* Создаем LVGL дисплей и полный буфер */
    static lv_color_t *buf1 = NULL;
    size_t buf_pixels = LCD_H_RES * LCD_V_RES;
    buf1 = heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf1) {
        buf1 = heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }

    lv_init();
    s_lv_display = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_color_format(s_lv_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(s_lv_display, lvgl_flush_cb);
    lv_display_set_buffers(s_lv_display, buf1, NULL, buf_pixels * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_FULL);
    // lv_display_set_antialiasing(s_lv_display, false);
    ESP_LOGI("LVGL", "lv_color_t = %d bytes", (int)sizeof(lv_color_t));

    /* Тикер LVGL */
    const esp_timer_create_args_t tick_args = {
        .callback = &lvgl_tick_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lv_tick"
    };
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &s_lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_lvgl_tick_timer, LVGL_TICK_MS * 1000));
}

void display_set_brightness(uint8_t percent)
{
    if (!s_bl_inited) return;
    /* В режиме без PWM просто включаем/выключаем подсветку */
    gpio_set_level(LCD_BL_GPIO, percent > 0 ? 1 : 0);
}


