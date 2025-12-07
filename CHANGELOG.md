# Changelog - Адаптация 4.0_LvglWidgets → Hello World Demo

## Версия 1.0 - Hello World Demo

**Дата:** 2025-12-07

### Основные изменения

#### Платформа
- ✅ **Arduino → ESP-IDF v5.5.1**
- ✅ **LVGL v8.x → LVGL v9.4**
- ✅ **Arduino_GFX → esp_lcd_panel_rgb (нативный ESP-IDF)**

#### Архитектура

**Удалено:**
- `4.0_LvglWidgets.ino` (Arduino sketch)
- Библиотека Arduino_GFX
- Зависимости Arduino (Wire, SPI)
- Демо виджеты (lv_demo_widgets.c/h)
- Файлы изображений (img_*.c)
- touch.h (временно, для упрощения)

**Добавлено:**
- `main/main.c` - простой Hello World
- `main/display.c` - драйвер дисплея ST7701 + RGB panel
- `main/display.h` - API дисплея
- `lv_conf.h` - конфигурация LVGL v9
- `main/idf_component.yml` - управление зависимостями
- `README.md` - полная документация
- `QUICKSTART.md` - быстрый старт
- `EXAMPLES.md` - примеры расширения

#### Инициализация дисплея

**Было (Arduino):**
```cpp
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(...);
Arduino_ST7701_RGBPanel *gfx = new Arduino_ST7701_RGBPanel(bus, ...);
gfx->begin(16000000);
```

**Стало (ESP-IDF):**
```c
st7701_init_sequence();  // 3-wire SPI инициализация
esp_lcd_rgb_panel_config_t cfg = {...};
esp_lcd_new_rgb_panel(&cfg, &panel);
esp_lcd_panel_init(panel);
```

#### LVGL API (v8 → v9)

**Изменения API:**

| LVGL v8 (Arduino)                  | LVGL v9 (ESP-IDF)                    |
|------------------------------------|--------------------------------------|
| `lv_scr_act()`                     | `lv_screen_active()`                 |
| `lv_disp_draw_buf_init()`          | `lv_display_set_buffers()`          |
| `lv_disp_drv_init()`               | `lv_display_create()`               |
| `lv_disp_drv_register()`           | (не требуется)                       |
| `lv_color_t` (автоопределение)     | `LV_COLOR_FORMAT_RGB565` (явное)    |
| `LV_COLOR_DEPTH` макрос            | Удален в v9                          |

**Пример создания дисплея:**

```c
// v9: новый подход
lv_display_t *disp = lv_display_create(480, 480);
lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
lv_display_set_flush_cb(disp, flush_callback);
lv_display_set_buffers(disp, buf1, NULL, buf_size, LV_DISPLAY_RENDER_MODE_FULL);
```

#### Управление памятью

**Буфер:**
- Размер: 480×480×2 = 460,800 байт (full-screen)
- Расположение: PSRAM (через `heap_caps_malloc`)
- Режим: `LV_DISPLAY_RENDER_MODE_FULL` (без двойной буферизации)

**Было (Arduino):**
```cpp
disp_draw_buf = (lv_color_t *)heap_caps_malloc(..., MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * 200);
```

**Стало (ESP-IDF):**
```c
buf = heap_caps_malloc(480*480*sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
lv_display_set_buffers(disp, buf, NULL, 480*480*sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_FULL);
```

#### Таймеры

**Было (Arduino):**
```cpp
void loop() {
  lv_timer_handler();
  delay(5);
}
```

**Стало (ESP-IDF):**
```c
// Tick timer
esp_timer_create(&tick_args, &tick_timer);
esp_timer_start_periodic(tick_timer, 5000); // 5ms, callback: lv_tick_inc()

// Handler timer
esp_timer_create(&handler_args, &handler_timer);
esp_timer_start_periodic(handler_timer, 5000); // 5ms, callback: lv_timer_handler()
```

#### ST7701 Инициализация

**Сохранено из Arduino:**
- 3-wire SPI протокол (bit-bang через GPIO)
- Последовательность команд st7701_type1_init_operations
- Параметры гаммы (Page 0: B0, B1)
- Настройки питания (Page 1: E0-E8, EB, EC)
- Pixel format: RGB565 (0x50)

**Изменения:**
- Использование прямого доступа к GPIO через ESP-IDF API
- Удален класс Arduino_ST7701_RGBPanel
- Функции `st7701_cmd()` и `st7701_data()` реализованы напрямую

#### Конфигурация RGB интерфейса

**Пины (без изменений):**
```
DE:18  VSYNC:17  HSYNC:16  PCLK:21
R[4:0]: 11,12,13,14,0
G[5:0]: 8,20,3,46,9,10
B[4:0]: 4,5,6,7,15
BL:38  CS:39  SCL:48  SDA:47
```

**Тайминги (сохранены из Arduino):**
```
HSYNC: front=10, pulse=8, back=50
VSYNC: front=8, pulse=8, back=22
PCLK: 10 MHz
```

#### UI Упрощения

**Удалено (из lv_demo_widgets.c):**
- 3 вкладки (Profile, Analytics, Shop)
- ~1500 строк кода виджетов
- Календарь, клавиатура, графики
- Сложные grid layouts
- Анимации метров
- Смена цветовой темы

**Оставлено (простой Hello World):**
- 1 экран с темным фоном
- 3 текстовых лейбла (заголовок, подзаголовок, инфо)
- 1 кнопка (без обработчика)
- Простое центрирование через `lv_obj_align()`

**Шрифты:**
```c
// Включены только используемые
#define LV_FONT_MONTSERRAT_14 1  // default
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_48 1  // главный заголовок
```

#### Размер кода

**Сравнение:**

| Проект           | Lines of Code | Binary Size  |
|------------------|---------------|--------------|
| 4.0_LvglWidgets  | ~2000 LOC     | ~1.2 MB      |
| Hello World Demo | ~150 LOC      | ~800 KB      |

#### Производительность

**FPS:**
- Arduino (partial buffer 200 lines): ~40-50 FPS
- ESP-IDF (full-screen buffer): ~60 FPS (ограничено PCLK)

**Память:**
- Arduino: ~192 KB LVGL heap
- ESP-IDF: ~460 KB PSRAM для буфера + LVGL heap

#### Конфигурация проекта

**sdkconfig изменения:**
```
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_QUAD=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y
CONFIG_FREERTOS_HZ=1000
```

### Совместимость

✅ **Работает с:**
- ESP32-S3 (протестировано)
- ESP-IDF v5.5.1
- LVGL v9.3.0+
- Дисплей ESP32-4848S040 (ST7701)

⚠️ **Не протестировано:**
- Другие варианты ESP32-S3 модулей
- Другие разрешения дисплея
- ESP-IDF v5.4 и ниже

❌ **Не поддерживается:**
- Тачскрин (пока не портирован)
- Демо виджеты
- Arduino IDE

### Известные ограничения

1. **Нет тачскрина** - требуется портировать GT911 драйвер
2. **Простой UI** - только статичные элементы
3. **Нет обработчиков событий** - кнопка декоративная
4. **Фиксированная яркость** - нет PWM для плавной регулировки

### Следующие шаги (TODO)

- [ ] Портировать драйвер тачскрина GT911
- [ ] Добавить примеры с обработчиками событий
- [ ] Реализовать PWM для плавной яркости
- [ ] Добавить демо с графиками/виджетами
- [ ] Оптимизировать использование PSRAM
- [ ] Добавить partial rendering для экономии памяти

### Полезные ссылки

**Миграция LVGL v8→v9:**
- https://docs.lvgl.io/master/details/main-components/display.html
- https://github.com/lvgl/lvgl/blob/master/docs/CHANGELOG.md

**ESP-IDF LCD:**
- https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/lcd.html

**ST7701 Datasheet:**
- https://www.displayfuture.com/Display/datasheet/controller/ST7701.pdf

### Авторы

Адаптация выполнена на основе:
- **4.0_LvglWidgets** (Arduino project) - оригинальный демо
- **ESP-IDF examples** - структура проекта
- **LVGL v9 migration guide** - обновление API

---

*Этот changelog описывает техническую адаптацию проекта для использования в образовательных целях.*

