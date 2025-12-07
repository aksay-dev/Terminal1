# Отладка тачскрина GT911

## ✅ Что было исправлено

### Проблема
```
[Warn] indev_pointer_proc: X is -7168 which is smaller than zero
[Warn] indev_pointer_proc: Y is -19456 which is smaller than zero
```

### Причины и исправления

1. **❌ Неправильный адрес чтения данных**
   - Было: `GT911_REG_DATA + 1` (неопределенный адрес)
   - Стало: `0x814F` (правильный регистр точки касания)

2. **❌ Неправильный порядок байтов**
   - Было: `x = (data[1] << 8) | data[0]` (big-endian)
   - Стало: `x = data[0] | (data[1] << 8)` (little-endian)

3. **❌ Нужна инверсия оси X**
   - Добавлено: `touch_last_x = TOUCH_MAX_X - x_raw`

4. **❌ Отсутствие валидации**
   - Добавлены проверки диапазона координат
   - Добавлено ограничение (clamp) значений

5. **❌ Мало логирования**
   - Добавлено подробное логирование инициализации
   - Добавлены логи при чтении невалидных данных

## 🔧 Как проверить исправления

### 1. Пересоберите проект

```bash
cd C:\_AAA\ESP\Terminal1
idf.py build flash monitor
```

### 2. Проверьте логи инициализации

Должны увидеть:

```
I (xxx) GT911: Initializing GT911 touchscreen...
I (xxx) GT911: I2C SDA: GPIO19, SCL: GPIO45
I (xxx) GT911: I2C driver installed successfully
I (xxx) GT911: Reading Product ID from address 0x5D...
I (xxx) GT911: ✓ GT911 detected at I2C addr 0x5D
I (xxx) GT911: ✓ Product ID: 9911
I (xxx) GT911: ✓ Resolution: 480 x 480
I (xxx) GT911: ✓ Firmware: 0xXXXX
I (xxx) GT911: GT911 initialization complete!
I (xxx) HELLO_WORLD: Touchscreen GT911 initialized successfully
```

### 3. Проверьте работу тачскрина

**Нажмите на кнопку "Press Me!":**
- Счетчик должен увеличиваться
- Кнопка должна менять цвет
- НЕ должно быть warning'ов про отрицательные координаты

## ❌ Если все еще не работает

### Проблема 1: GT911 не обнаружен

```
E (xxx) GT911: ✗ GT911 not found on I2C bus!
E (xxx) GT911:   Check wiring: SDA=GPIO19, SCL=GPIO45
```

**Решение:**

1. **Проверьте физическое подключение:**
   ```
   GT911 SDA → ESP32 GPIO19
   GT911 SCL → ESP32 GPIO45
   GT911 VCC → 3.3V
   GT911 GND → GND
   ```

2. **Проверьте что провода не перепутаны**

3. **Сканируйте I2C bus:**
   
   Добавьте в `touch_init()` перед чтением Product ID:
   
   ```c
   ESP_LOGI(TAG, "Scanning I2C bus...");
   for (uint8_t addr = 1; addr < 127; addr++) {
       i2c_cmd_handle_t cmd = i2c_cmd_link_create();
       i2c_master_start(cmd);
       i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
       i2c_master_stop(cmd);
       esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 50);
       i2c_cmd_link_delete(cmd);
       if (ret == ESP_OK) {
           ESP_LOGI(TAG, "  Found device at address: 0x%02X", addr);
       }
   }
   ```

### Проблема 2: Координаты все еще неправильные

**Если X/Y перепутаны:**

В `touch.c`, функция `touch_touched()`:
```c
// Поменяйте местами:
touch_last_x = y_raw;
touch_last_y = x_raw;
```

**Если нужна другая инверсия:**

```c
// Инверсия только X:
touch_last_x = TOUCH_MAX_X - x_raw;
touch_last_y = y_raw;

// Инверсия только Y:
touch_last_x = x_raw;
touch_last_y = TOUCH_MAX_Y - y_raw;

// Инверсия обоих:
touch_last_x = TOUCH_MAX_X - x_raw;
touch_last_y = TOUCH_MAX_Y - y_raw;

// Без инверсии:
touch_last_x = x_raw;
touch_last_y = y_raw;
```

### Проблема 3: Кнопка не реагирует

**Добавьте отладку в callback:**

```c
static void touchpad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (touch_has_signal()) {
        if (touch_touched()) {
            data->state = LV_INDEV_STATE_PRESSED;
            data->point.x = touch_last_x;
            data->point.y = touch_last_y;
            
            // ОТЛАДКА: Логировать каждое касание
            ESP_LOGI("TOUCH", "Touch at: x=%d, y=%d", touch_last_x, touch_last_y);
        } else {
            data->state = LV_INDEV_STATE_RELEASED;
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
```

Нажмите на экран и проверьте координаты в логах.

### Проблема 4: Спам в логах

Если видите много повторяющихся сообщений, снизьте частоту логирования:

```c
// В touchpad_read_cb, логируйте только при изменении:
static int16_t last_logged_x = -1;
static int16_t last_logged_y = -1;

if (touch_touched()) {
    if (touch_last_x != last_logged_x || touch_last_y != last_logged_y) {
        ESP_LOGI("TOUCH", "Touch at: x=%d, y=%d", touch_last_x, touch_last_y);
        last_logged_x = touch_last_x;
        last_logged_y = touch_last_y;
    }
}
```

## 📊 Диагностические команды

### Проверка I2C

В Linux/Mac:
```bash
i2cdetect -y 0
```

В ESP-IDF (через компонент i2c_tools):
```bash
idf.py menuconfig
# Component config → I2C Tools → Enable I2C Tools
idf.py build flash

# В мониторе:
i2cdetect
```

### Чтение регистров GT911 вручную

Добавьте тестовую функцию:

```c
void gt911_dump_regs(void)
{
    uint8_t data[16];
    
    ESP_LOGI(TAG, "=== GT911 Registers ===");
    
    // Product ID
    gt911_read_reg(0x8140, data, 4);
    ESP_LOGI(TAG, "Product ID: %c%c%c%c", data[0], data[1], data[2], data[3]);
    
    // Config version
    gt911_read_reg(0x8047, data, 1);
    ESP_LOGI(TAG, "Config Ver: 0x%02X", data[0]);
    
    // Resolution
    gt911_read_reg(0x8048, data, 4);
    ESP_LOGI(TAG, "X Resolution: %d", data[0] | (data[1] << 8));
    ESP_LOGI(TAG, "Y Resolution: %d", data[2] | (data[3] << 8));
    
    // Touch points
    gt911_read_reg(0x814E, data, 1);
    ESP_LOGI(TAG, "Touch Status: 0x%02X (points: %d)", data[0], data[0] & 0x0F);
}
```

Вызовите в `touch_init()` после успешной инициализации.

## ✅ Контрольный список

- [ ] GT911 обнаружен (Product ID = 9911)
- [ ] Разрешение = 480 x 480
- [ ] При касании НЕТ warning'ов про отрицательные координаты
- [ ] Координаты в диапазоне 0-479
- [ ] Кнопка реагирует на нажатие
- [ ] Счетчик увеличивается
- [ ] Цвет кнопки меняется

## 📚 Дополнительные ресурсы

**GT911 Datasheet:**
- https://github.com/goodix/gt9xx_driver_generic

**Регистры GT911:**
```
0x8140 - Product ID (4 bytes): "9911"
0x8144 - Firmware Version (2 bytes)
0x8047 - Config Version (1 byte)
0x8048 - X Resolution (2 bytes, little-endian)
0x804A - Y Resolution (2 bytes, little-endian)
0x814E - Touch Status (1 byte)
         Bit 7: Buffer status (1=ready)
         Bit 3-0: Number of touch points
0x814F - Point 1 data (8 bytes)
         [0-1]: X coordinate (little-endian)
         [2-3]: Y coordinate (little-endian)
         [4-5]: Size
         [6-7]: Reserved
```

## 🎯 Ожидаемый результат

После исправлений вы должны видеть:

```
I (xxx) GT911: GT911 initialization complete!
I (xxx) HELLO_WORLD: Touchscreen input device registered
I (xxx) HELLO_WORLD: Touch the button on the display!

# При касании кнопки:
I (xxx) HELLO_WORLD: Button clicked! Total clicks: 1
I (xxx) HELLO_WORLD: Button clicked! Total clicks: 2
...
```

**НЕТ** предупреждений про отрицательные координаты!

---

Если проблемы продолжаются, пришлите полный лог с момента загрузки до первого касания.

