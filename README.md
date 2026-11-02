# AlashDS18B20

[Русский](#русский) | [Қазақша](#қазақша) | [English](#english)

---

## Русский

Простая библиотека для датчиков температуры **DS18B20** (а также DS18S20 и DS1822). Сделана для уроков и наборов Alash Electronics: один вызов `begin()`, несколько датчиков на одном проводе, понятные ошибки. **Не нужны** библиотеки OneWire и DallasTemperature.

```cpp
#include <AlashDS18B20.h>
AlashDS18B20 sensors(4);          // пин данных

void setup() {
  Serial.begin(115200);
  if (!sensors.begin()) { Serial.println(sensors.errorText()); while (true); }
}
void loop() {
  Serial.println(sensors.temperature());   // °C
  delay(1000);
}
```

### Что умеет
- **Сам находит все датчики** на линии: адрес знать не нужно. `count()` скажет сколько, `temperature(0)`, `temperature(1)`... дадут температуру каждого.
- **Все датчики измеряют одновременно**: ждать 0,75 с надо один раз, а не на каждый датчик.
- **Без остановки программы:** `request()` запускает измерение, `ready()` говорит, когда оно закончилось (пример `NonBlocking`).
- **Не принимает ошибку за температуру:** проверяет CRC, повторяет чтение при сбое, значение 85 °C (датчик без нормального питания) перепроверяет. Если не получилось, вернёт `NAN`, а не 0 или -127.
- **Понятные ошибки** на русском, казахском и английском: `errorText()` и `diagnose(Serial)` (покажет линию, найденные датчики с адресами и что проверить).
- **Единицы:** `temperature()` в °C, `temperatureF()` в °F; `temperatureByAddress()` по адресу; `setResolution(9..12)` меняет точность и скорость.
- **Работает на Arduino AVR (Uno, Nano, Mega), ESP32, ESP8266** и других платах. Никаких внешних библиотек.

### Подключение
| Датчик (3 провода) | Uno / Nano | ESP32 |
|---|---|---|
| красный (VCC) | 5 В | 3.3 В |
| чёрный (GND) | GND | GND |
| жёлтый (DATA) | D4 | GPIO4 |

Между жёлтым и красным проводом нужен **резистор 4,7 кОм**. Без него датчик не найдётся. На ESP32 не используйте пины 34-39: они только на вход.

### Чем отличается от других библиотек
Мы посмотрели DallasTemperature (Miles Burton), OneWire (Paul Stoffregen), DS18B20 (Matmunk) и DS18B20_RT (Rob Tillaart):
- DallasTemperature требует установить ещё и OneWire, а новичок не знает про вторую библиотеку и получает ошибку компиляции. У нас одна библиотека.
- В них без `requestTemperatures()` и `getTempCByIndex()` (две строки подряд) не обойтись, а при отключённом датчике возвращается -127. У нас `temperature()` одним вызовом, а отказ это `NAN` и объяснение.
- Нет подсказок, что делать, если датчик не нашёлся: у нас `errorText()` и `diagnose()`.
- Несколько датчиков на одном проводе есть везде; у нас ещё общий запуск измерения и выбор по номеру или по адресу.

DallasTemperature по-прежнему хороша, если нужны будильники (alarm) и запись в EEPROM датчика: в AlashDS18B20 их нет (датчик с паразитным питанием тоже не поддерживается: подключайте три провода).

### Примеры
`Basic`, `Diagnose`, `MultipleSensors`, `NonBlocking`, `Thermostat`.

### Тесты
Протокол 1-Wire (поиск устройств по AN187, CRC8, выбор датчика, расчёт температуры) проверен без железа на имитации линии: `extras/test`.

Подробное руководство: [docs/WIKI.md](docs/WIKI.md).

---

## Қазақша

**DS18B20** температура сенсорларына (сондай-ақ DS18S20 және DS1822) арналған қарапайым кітапхана. Alash Electronics сабақтары мен жинақтарына арналған: бір `begin()` шақыруы, бір сымда бірнеше сенсор, түсінікті қателер. OneWire және DallasTemperature кітапханалары **қажет емес**.

```cpp
#include <AlashDS18B20.h>
AlashDS18B20 sensors(4);          // дерек пині

void setup() {
  Serial.begin(115200);
  if (!sensors.begin()) { Serial.println(sensors.errorText()); while (true); }
}
void loop() {
  Serial.println(sensors.temperature());   // °C
  delay(1000);
}
```

### Мүмкіндіктері
- **Желідегі барлық сенсорды өзі табады:** мекенжайды білудің қажеті жоқ. `count()` санын, `temperature(0)`, `temperature(1)`... әрқайсысының температурасын береді.
- **Барлық сенсор бір мезгілде өлшейді:** 0,75 с күту бір рет қана керек.
- **Бағдарламаны тоқтатпай:** `request()` өлшеуді бастайды, `ready()` оның аяқталғанын айтады (`NonBlocking` мысалы).
- **Қатені температура деп қабылдамайды:** CRC тексереді, ақау кезінде қайта оқиды, 85 °C мәнін (қуаты дұрыс емес сенсор) қайта тексереді. Болмаса 0 немесе -127 емес, `NAN` қайтарады.
- **Түсінікті қателер** орысша, қазақша және ағылшынша: `errorText()` және `diagnose(Serial)`.
- **Бірліктер:** `temperature()` °C, `temperatureF()` °F; `temperatureByAddress()` мекенжай бойынша; `setResolution(9..12)` дәлдік пен жылдамдықты өзгертеді.
- **Arduino AVR (Uno, Nano, Mega), ESP32, ESP8266** тақталарында жұмыс істейді. Сыртқы кітапхана қажет емес.

### Қосу
| Сенсор (3 сым) | Uno / Nano | ESP32 |
|---|---|---|
| қызыл (VCC) | 5 В | 3.3 В |
| қара (GND) | GND | GND |
| сары (DATA) | D4 | GPIO4 |

Сары және қызыл сым арасына **4,7 кОм резистор** қою керек. Онсыз сенсор табылмайды. ESP32-де 34-39 пиндерін қолданбаңыз: олар тек кіріс.

### Мысалдар
`Basic`, `Diagnose`, `MultipleSensors`, `NonBlocking`, `Thermostat`.

Толық нұсқаулық: [docs/WIKI.md](docs/WIKI.md).

---

## English

A simple library for **DS18B20** temperature sensors (also DS18S20 and DS1822). Made for classes and Alash Electronics kits: one `begin()` call, several sensors on one wire, clear error messages. **No OneWire or DallasTemperature needed.**

```cpp
#include <AlashDS18B20.h>
AlashDS18B20 sensors(4);          // data pin

void setup() {
  Serial.begin(115200);
  if (!sensors.begin()) { Serial.println(sensors.errorText()); while (true); }
}
void loop() {
  Serial.println(sensors.temperature());   // °C
  delay(1000);
}
```

### Features
- **Finds all sensors itself:** no need to know addresses. `count()` tells how many, `temperature(0)`, `temperature(1)`... return each reading.
- **All sensors measure at once:** you wait 0.75 s once, not per sensor.
- **Non-blocking:** `request()` starts a measurement, `ready()` tells when it is done (`NonBlocking` example).
- **Never mistakes an error for a temperature:** CRC check, retries on failure, re-measures the 85 °C power-on value. On failure you get `NAN`, not 0 or -127.
- **Clear errors** in Russian, Kazakh and English: `errorText()` and `diagnose(Serial)`.
- **Units:** `temperature()` in °C, `temperatureF()` in °F; `temperatureByAddress()`; `setResolution(9..12)` trades precision for speed.
- Works on **Arduino AVR (Uno, Nano, Mega), ESP32, ESP8266** and others, with no extra libraries.

### Wiring
| Sensor (3 wires) | Uno / Nano | ESP32 |
|---|---|---|
| red (VCC) | 5 V | 3.3 V |
| black (GND) | GND | GND |
| yellow (DATA) | D4 | GPIO4 |

A **4.7 kOhm resistor** between the yellow and red wires is required. On ESP32 avoid pins 34-39: they are input-only.

Not supported: alarms, EEPROM copy and parasite power (use the three-wire connection). Use DallasTemperature if you need those.

### Examples
`Basic`, `Diagnose`, `MultipleSensors`, `NonBlocking`, `Thermostat`.

Full guide: [docs/WIKI.md](docs/WIKI.md).

### License
MIT, Alash Electronics.
