# Архитектура проекта Hello World LVGL v9.4

## 🏗️ Общая схема

```
┌─────────────────────────────────────────────────────────────┐
│                     ESP32-S3 Application                     │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────────────┐         ┌──────────────────┐          │
│  │   main.c        │         │   display.c      │          │
│  │                 │         │                  │          │
│  │ • UI Creation   │────────▶│ • ST7701 Init   │          │
│  │ • Event Loop    │         │ • RGB Panel     │          │
│  │ • App Logic     │         │ • Flush CB      │          │
│  └─────────────────┘         └──────────────────┘          │
│           │                           │                      │
│           │                           │                      │
│           ▼                           ▼                      │
│  ┌──────────────────────────────────────────────┐          │
│  │            LVGL v9.3/9.4                     │          │
│  │  • Rendering Engine                          │          │
│  │  • Widget Management                         │          │
│  │  • Event System                              │          │
│  │  • Timer Handler                             │          │
│  └──────────────────────────────────────────────┘          │
│           │                                                  │
│           ▼                                                  │
│  ┌──────────────────────────────────────────────┐          │
│  │        ESP-IDF HAL (v5.5.1)                  │          │
│  │  • esp_lcd_panel_rgb                         │          │
│  │  • esp_timer                                 │          │
│  │  • GPIO Driver                               │          │
│  │  • PSRAM Manager                             │          │
│  └──────────────────────────────────────────────┘          │
│           │                                                  │
└───────────┼──────────────────────────────────────────────────┘
            │
            ▼
    ┌───────────────────┐
    │  ST7701 Display   │
    │  480x480 RGB565   │
    └───────────────────┘
```

## 📦 Компоненты проекта

### 1. Application Layer (main.c)

**Ответственность:**
- Создание UI элементов
- Обработка пользовательских событий
- Бизнес-логика приложения

**Основные функции:**
```c
void app_main(void)
  ├─ display_init()          // Инициализация дисплея
  ├─ esp_timer_create()      // Создание таймера LVGL
  ├─ lv_screen_active()      // Получение активного экрана
  ├─ lv_label_create()       // Создание UI элементов
  └─ display_set_brightness()// Установка яркости
```

**Зависимости:**
- `display.h` - API дисплея
- `lvgl.h` - LVGL библиотека
- `esp_timer.h` - ESP-IDF таймеры

### 2. Display Driver Layer (display.c)

**Ответственность:**
- Инициализация контроллера ST7701
- Настройка RGB интерфейса
- Управление подсветкой
- Flush callback для LVGL

**Основные функции:**
```c
void display_init(void)
  ├─ st7701_gpio_init()      // Инициализация GPIO для 3-wire SPI
  ├─ st7701_init_sequence()  // Отправка команд инициализации ST7701
  ├─ esp_lcd_new_rgb_panel() // Создание RGB панели
  ├─ lv_init()               // Инициализация LVGL
  ├─ lv_display_create()     // Создание LVGL дисплея
  ├─ lv_display_set_buffers()// Привязка буфера
  └─ esp_timer_create()      // Таймер для lv_tick_inc()

void display_set_brightness(uint8_t percent)
  └─ gpio_set_level()        // Управление GPIO38 (BL)

void lvgl_flush_cb(...)
  └─ esp_lcd_panel_draw_bitmap() // Отправка данных на дисплей
```

**Внутренние функции:**
```c
// 3-wire SPI протокол
static void st7701_write9(uint8_t dc, uint8_t data)
static void st7701_cmd(uint8_t cmd)
static void st7701_data(uint8_t data)

// Таймер тиков LVGL
static void lvgl_tick_cb(void *arg)
```

### 3. LVGL Library

**Внешняя зависимость:**
- Подключается через `idf_component.yml`
- Версия: ^9.3.0 (совместима с v9.4)

**Конфигурация:**
- `lv_conf.h` - настройки цвета, памяти, виджетов

**Основные модули:**
```
LVGL Core
  ├─ Display Management
  ├─ Input Device Management
  ├─ Rendering Engine (lv_draw_sw)
  ├─ Memory Management
  ├─ Timer System
  └─ Widget Library
```

### 4. ESP-IDF HAL

**Используемые модули:**

| Модуль              | Применение                            |
|---------------------|---------------------------------------|
| `esp_lcd_panel_rgb` | RGB параллельный интерфейс            |
| `esp_timer`         | Высокоточные таймеры                  |
| `driver/gpio`       | Управление GPIO (BL, SPI)             |
| `esp_heap_caps`     | Аллокация памяти в PSRAM              |
| `esp_log`           | Логирование                           |

## 🔄 Поток данных

### Инициализация (startup)

```
Power On
    │
    ▼
app_main()
    │
    ├─▶ display_init()
    │       │
    │       ├─▶ ST7701 Init (3-wire SPI)
    │       │       └─▶ Send init commands
    │       │
    │       ├─▶ RGB Panel Init
    │       │       ├─▶ Configure timings
    │       │       └─▶ Map GPIO pins
    │       │
    │       ├─▶ Allocate framebuffer (PSRAM)
    │       │
    │       └─▶ LVGL Init
    │               ├─▶ lv_init()
    │               ├─▶ Create display
    │               ├─▶ Set flush callback
    │               └─▶ Start tick timer
    │
    ├─▶ Create LVGL timer (handler)
    │
    └─▶ Create UI
            ├─▶ Set background
            ├─▶ Create labels
            └─▶ Create button
```

### Runtime Loop

```
                  ┌─────────────────┐
                  │  FreeRTOS Task  │
                  │   (ESP-IDF)     │
                  └────────┬────────┘
                           │
        ┌──────────────────┴──────────────────┐
        │                                     │
        ▼                                     ▼
┌───────────────┐                    ┌───────────────┐
│  Tick Timer   │                    │ Handler Timer │
│   (5ms)       │                    │    (5ms)      │
└───────┬───────┘                    └───────┬───────┘
        │                                    │
        ▼                                    ▼
  lv_tick_inc(5)                      lv_timer_handler()
        │                                    │
        │                                    │
        │            ┌───────────────────────┤
        │            │                       │
        │            ▼                       ▼
        │    ┌───────────────┐      ┌──────────────┐
        │    │  User Timers  │      │  UI Refresh  │
        │    │  (if any)     │      │  (on change) │
        │    └───────────────┘      └───────┬──────┘
        │                                   │
        └───────────────────────────────────┤
                                            │
                                            ▼
                                   ┌─────────────────┐
                                   │  Render Engine  │
                                   │   (lv_draw_sw)  │
                                   └────────┬────────┘
                                            │
                                            ▼
                                   ┌─────────────────┐
                                   │  lvgl_flush_cb  │
                                   └────────┬────────┘
                                            │
                                            ▼
                                  esp_lcd_panel_draw_bitmap()
                                            │
                                            ▼
                                   ┌─────────────────┐
                                   │  RGB Interface  │
                                   │   (Hardware)    │
                                   └────────┬────────┘
                                            │
                                            ▼
                                      ST7701 Display
```

## 💾 Управление памятью

### Распределение памяти

```
ESP32-S3 Memory Map
├── Internal SRAM (512 KB)
│   ├── Code & Data                    ~100 KB
│   ├── FreeRTOS Heap                  ~200 KB
│   ├── LVGL Heap (internal)           ~20 KB
│   └── Stack                          ~32 KB
│
├── External PSRAM (8 MB)
│   ├── LVGL Framebuffer (480×480×2)   460 KB
│   ├── LVGL Heap (external)           ~1 MB
│   └── Available                      ~6.5 MB
│
└── Flash (16 MB)
    ├── Application Binary              ~800 KB
    ├── Partition Table                 4 KB
    └── NVS / OTA / etc                 ~15 MB
```

### Framebuffer

**Размер:** 480 × 480 × 2 bytes = 460,800 bytes

**Расположение:** PSRAM

**Режим:** Full-screen single buffer
- Весь экран рендерится за один проход
- Нет двойной буферизации (экономия памяти)
- Flush callback отправляет данные напрямую в RGB интерфейс

**Аллокация:**
```c
lv_color_t *buf = heap_caps_malloc(
    480 * 480 * sizeof(lv_color_t), 
    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
);
```

## ⚙️ Конфигурация

### lv_conf.h (ключевые настройки)

```c
// Цвет
#define LV_COLOR_DEPTH 16              // RGB565

// Шрифты (только используемые)
#define LV_FONT_MONTSERRAT_14 1        // Default
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_48 1        // Главный заголовок

// Виджеты
#define LV_USE_LABEL     1
#define LV_USE_BUTTON    1

// Логирование
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO
```

### sdkconfig (ESP-IDF)

```ini
# PSRAM
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_QUAD=y

# CPU
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y

# FreeRTOS
CONFIG_FREERTOS_HZ=1000

# PSRAM allocation
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384
```

## 🔌 Hardware Interface

### ST7701 Control (3-wire SPI, bit-bang)

```
ESP32-S3          ST7701
  GPIO39 ────────▶ CS
  GPIO48 ────────▶ SCL
  GPIO47 ────────▶ SDA
  
Protocol: 9-bit frames
  - 1st bit: D/C (0=command, 1=data)
  - Next 8 bits: payload
  
Timing: ~1µs per clock pulse
```

### RGB Interface (16-bit parallel)

```
ESP32-S3           ST7701
  GPIO16  ────────▶ HSYNC
  GPIO17  ────────▶ VSYNC
  GPIO18  ────────▶ DE (Data Enable)
  GPIO21  ────────▶ PCLK (Pixel Clock)
  
  GPIO[11:14,0]  ─▶ R[4:0] (Red 5-bit)
  GPIO[8,20,3,46,9,10] ─▶ G[5:0] (Green 6-bit)
  GPIO[4:7,15]   ─▶ B[4:0] (Blue 5-bit)

Timings:
  Pixel Clock: 10 MHz
  HSYNC: Front=10, Pulse=8, Back=50
  VSYNC: Front=8, Pulse=8, Back=22
```

### Backlight Control

```
ESP32-S3          Display BL
  GPIO38 ────────▶ Backlight Enable
  
Current: Simple ON/OFF (no PWM)
Future: LEDC PWM for brightness control
```

## 🎨 UI Architecture

### Screen Hierarchy

```
lv_screen_active()
    │
    ├─ lbl_hello        ("Hello, World!")
    │   └─ Style: 48pt green, centered(-80px)
    │
    ├─ lbl_subtitle     ("LVGL v9.4 on ESP32-S3")
    │   └─ Style: 24pt white, centered
    │
    ├─ lbl_info         ("Resolution: 480x480...")
    │   └─ Style: 18pt gray, centered(+80px)
    │
    └─ btn              (Button)
        └─ btn_label    ("Press Me!")
            └─ Style: 20pt, blue bg, rounded
```

### Alignment System

```
Screen (480×480)
┌─────────────────────────────────────┐
│                                     │  ← y=-80
│         Hello, World!               │
│                                     │  ← y=0 (center)
│   LVGL v9.4 on ESP32-S3            │
│                                     │  ← y=+80
│   Resolution: 480 x 480            │
│                                     │
│                                     │  ← y=-40 from bottom
│       [ Press Me! ]                │
│                                     │
└─────────────────────────────────────┘
```

## 🔍 Debug & Logging

### Log Levels

```
ESP_LOGI("TAG", ...)  ─▶  [INFO]  Application events
ESP_LOGW("TAG", ...)  ─▶  [WARN]  Potential issues
ESP_LOGE("TAG", ...)  ─▶  [ERROR] Critical errors
```

### Key Log Points

```c
// main.c
ESP_LOGI("HELLO_WORLD", "=== Demo Starting ===");
ESP_LOGI("HELLO_WORLD", "Display initialized");
ESP_LOGI("HELLO_WORLD", "UI created");

// display.c
ESP_LOGI("LVGL", "lv_color_t = %d bytes", sizeof(lv_color_t));
ESP_LOGI("Display", "ST7701 initialized");
ESP_LOGI("Display", "RGB panel created");
```

## 🚀 Performance

### Rendering Pipeline

```
UI Change Detected
    │
    ▼
Mark dirty areas
    │
    ▼
lv_timer_handler()
    │
    ├─▶ Calculate changes
    │
    ├─▶ Render to framebuffer
    │       └─ lv_draw_sw (software rendering)
    │
    └─▶ lvgl_flush_cb()
            └─ esp_lcd_panel_draw_bitmap()
                    └─ DMA transfer to RGB interface
                            └─ Display updates
                            
Time: ~16ms per full-screen refresh (60 FPS)
```

### CPU Load

- Idle: ~5% (только таймеры)
- Rendering: ~40% (full-screen update)
- Average: ~10-15%

## 📈 Scalability

### Current Limits

- ✅ Single screen
- ✅ Static UI
- ✅ 4 UI elements
- ⚠️ Full-screen buffer (460KB)

### Optimization Options

1. **Partial rendering** - уменьшить буфер до 1/10 экрана
2. **Double buffering** - плавность за счет памяти
3. **Hardware acceleration** - если поддерживается MCU
4. **Compress images** - для больших картинок

## 🛠️ Extensibility Points

Проект легко расширить:

1. **Add touchscreen** → port `touch.h` from Arduino
2. **Add widgets** → enable in `lv_conf.h`
3. **Add animations** → `lv_anim_*` functions
4. **Add events** → `lv_obj_add_event_cb()`
5. **Add screens** → `lv_obj_create(NULL)`

См. `EXAMPLES.md` для готовых примеров!

---

**Архитектура оптимизирована для:**
- Простоты понимания ✅
- Легкости модификации ✅
- Обучающих целей ✅
- Быстрого прототипирования ✅

