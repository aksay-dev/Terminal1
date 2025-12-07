# Быстрый старт - Hello World Demo

## За 5 минут до запуска

### 1. Проверка окружения
```bash
# Проверка версии ESP-IDF
idf.py --version
# Должно быть: ESP-IDF v5.5.1 или новее

# Активация окружения (если еще не активировано)
. $IDF_PATH/export.sh  # Linux/Mac
%IDF_PATH%\export.bat  # Windows cmd
. $IDF_PATH/export.ps1 # Windows PowerShell
```

### 2. Сборка проекта
```bash
cd C:\_AAA\ESP\Terminal1

# Установка зависимостей (LVGL автоматически загрузится)
idf.py reconfigure

# Сборка
idf.py build
```

### 3. Прошивка
```bash
# Найдите ваш COM порт (COM3, COM5, и т.д. на Windows)
# Или /dev/ttyUSB0 на Linux

idf.py -p COM3 flash monitor
```

### 4. Результат
После прошивки на экране должно появиться:
- **"Hello, World!"** зеленым цветом (большой шрифт)
- **"LVGL v9.4 on ESP32-S3"** белым цветом
- Информация о разрешении экрана серым цветом
- Синяя кнопка **"Press Me!"** внизу

## Проверка в мониторе порта

Вы должны увидеть:
```
I (xxx) HELLO_WORLD: === Hello World LVGL v9.4 Demo ===
I (xxx) HELLO_WORLD: ESP-IDF: v5.5.1
I (xxx) HELLO_WORLD: Display initialized successfully
I (xxx) HELLO_WORLD: UI created successfully. Entering main loop...
I (xxx) HELLO_WORLD: Demo is running. Check your display!
```

## Если что-то не работает

### Проблема: "Failed to connect"
```bash
# Попробуйте другую скорость
idf.py -p COM3 -b 115200 flash

# Или нажмите и держите BOOT кнопку при подключении
```

### Проблема: Черный экран
1. Проверьте подключение дисплея
2. Проверьте питание (должно быть 5V на дисплей)
3. Яркость: код устанавливает 80%, попробуйте изменить на 100%

### Проблема: Компиляция не удается
```bash
# Очистка и пересборка
idf.py fullclean
rm -rf managed_components/  # Удалить загруженные компоненты
idf.py reconfigure
idf.py build
```

### Проблема: "PSRAM not found"
В `sdkconfig` должно быть:
```
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_QUAD=y
```

Проверьте через menuconfig:
```bash
idf.py menuconfig
# → Component config → ESP PSRAM → Support for external SPI-connected RAM
```

## Изменение текста

Откройте `main/main.c` и измените строку:
```c
lv_label_set_text(lbl_hello, "Hello, World!");
```

на что угодно, например:
```c
lv_label_set_text(lbl_hello, "Привет, Мир!");
```

Пересоберите и прошейте:
```bash
idf.py build flash
```

## Изменение цветов

В `main.c` найдите:
```c
/* Темный фон */
lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a2e), LV_PART_MAIN);

/* Зеленый текст */
lv_obj_set_style_text_color(lbl_hello, lv_color_hex(0x00ff88), LV_PART_MAIN);
```

Измените hex-коды цветов:
- `0xFF0000` - красный
- `0x00FF00` - зеленый
- `0x0000FF` - синий
- `0xFFFF00` - желтый
- `0xFF00FF` - пурпурный
- `0x00FFFF` - голубой

## Следующие шаги

1. **Добавить анимацию:** Изучите `lv_anim_*` функции
2. **Добавить обработчик кнопки:** Используйте `lv_obj_add_event_cb()`
3. **Создать несколько экранов:** `lv_obj_create()` и `lv_screen_load()`
4. **Добавить графики:** Включите `LV_USE_CHART` в `lv_conf.h`

## Полезные ссылки

- **LVGL Examples:** https://docs.lvgl.io/master/examples.html
- **Widget Documentation:** https://docs.lvgl.io/master/widgets/index.html
- **Styling Guide:** https://docs.lvgl.io/master/overview/style.html

## Поддержка

При возникновении проблем:
1. Проверьте README.md для детальной информации
2. Изучите секцию "Отладка" в README.md
3. Проверьте логи через `idf.py monitor`

