# AlashDS18B20: руководство / нұсқаулық / guide

[Русский](#русский) | [Қазақша](#қазақша) | [English](#english)

---

## Русский

Простая библиотека для датчиков температуры **DS18B20** (а также DS18S20 и DS1822). Сделана для уроков и наборов Alash Electronics: один вызов `begin()`, несколько датчиков на одном проводе, понятные ошибки. **Не нужны** библиотеки OneWire и DallasTemperature.

### Установка
Arduino IDE: Скетч, Подключить библиотеку, Управлять библиотеками, найти **AlashDS18B20** и нажать Установить. Или скачайте ZIP с GitHub: Скетч, Подключить библиотеку, Добавить .ZIP библиотеку.

### Платы
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 и другие, где работает Arduino-ядро.

### Подключение
| Датчик (3 провода) | Uno / Nano | ESP32 |
|---|---|---|
| красный (VCC) | 5 В | 3.3 В |
| чёрный (GND) | GND | GND |
| жёлтый (DATA) | D4 | GPIO4 |

Между жёлтым и красным проводом нужен **резистор 4,7 кОм**. Без него датчик не найдётся. На ESP32 не используйте пины 34-39: они только на вход.

### Быстрый старт
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
- **Паразитное питание (два провода):** `begin()` сам узнаёт, как питаются датчики, и на время измерения держит линию на питании; для больших линий есть `setPowerPin()` (ключ на полевом транзисторе).
- **Работает на Arduino AVR (Uno, Nano, Mega), ESP32, ESP8266** и других платах. Никаких внешних библиотек.

### Справочник по методам
| Метод | Что делает |
|---|---|
| `begin()` | Найти датчики на линии, поставить 12 бит |
| `temperature(i)` | Температура °C датчика номер i (NAN при ошибке) |
| `temperatureF(i)` | То же в °F |
| `temperatureByAddress(addr)` | Датчик по 8-байтовому адресу |
| `request() / ready()` | Измерение без ожидания: запустить, потом проверить |
| `count() / getAddress(i, a)` | Сколько датчиков и их адреса |
| `setResolution(bits)` | Точность 9-12 бит (быстрее или точнее) |
| `rescan()` | Искать датчики заново |
| `setCacheMs(ms)` | Сколько мс данные считаются свежими |
| `isParasite() / setPowerMode(mode)` | Питание датчиков: паразитное (два провода) определяется само; POWER_AUTO / EXTERNAL / PARASITE |
| `setPowerPin(pin, activeHigh)` | Ключ питания линии на полевом транзисторе (много датчиков, длинные провода) |
| `error() / errorText()` | Причина последней ошибки |
| `diagnose(Serial)` | Самопроверка |

### Если что-то не работает
**Датчик не найден (нет датчиков)**

Нужен резистор 4,7 кОм между сигнальным проводом (DQ) и питанием. Проверьте GND и VCC, провод DQ на нужном пине. Запустите пример Diagnose.

**Температура 85 °C или -127 °C**

85 °C это значение после включения, -127 °C означает обрыв линии. Библиотека сама повторяет измерение; если не проходит, проверьте провода и питание.

**Ошибка CRC**

Длинные провода, помехи или слабое питание. Сократите провод, поставьте резистор 4,7 кОм ближе к датчику, подайте питание на VCC (три провода). Если датчики паразитные (два провода), поставьте резистор 4,7 кОм на питание и подключите ключ питания `setPowerPin()`.

### Чем отличается от других библиотек
Мы посмотрели DallasTemperature (Miles Burton), OneWire (Paul Stoffregen), DS18B20 (Matmunk) и DS18B20_RT (Rob Tillaart):
- DallasTemperature требует установить ещё и OneWire, а новичок не знает про вторую библиотеку и получает ошибку компиляции. У нас одна библиотека.
- В них без `requestTemperatures()` и `getTempCByIndex()` (две строки подряд) не обойтись, а при отключённом датчике возвращается -127. У нас `temperature()` одним вызовом, а отказ это `NAN` и объяснение.
- Нет подсказок, что делать, если датчик не нашёлся: у нас `errorText()` и `diagnose()`.
- Несколько датчиков на одном проводе есть везде; у нас ещё общий запуск измерения и выбор по номеру или по адресу.

DallasTemperature по-прежнему хороша, если нужны будильники (alarm) и запись в EEPROM датчика: в AlashDS18B20 их нет (паразитное питание на двух проводах поддерживается, см. пример `ParasitePower`).

### Самопроверка
В библиотеке есть `diagnose(Serial)`: она печатает, что видит плата, и подсказывает, что проверить. Язык подсказок: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Примеры
`Basic`, `Diagnose`, `MultipleSensors`, `NonBlocking`, `ParasitePower`, `Thermostat`.

---

## Қазақша

**DS18B20** температура сенсорларына (сондай-ақ DS18S20 және DS1822) арналған қарапайым кітапхана. Alash Electronics сабақтары мен жинақтарына арналған: бір `begin()` шақыруы, бір сымда бірнеше сенсор, түсінікті қателер. OneWire және DallasTemperature кітапханалары **қажет емес**.

### Орнату
Arduino IDE: Sketch, Include Library, Manage Libraries, **AlashDS18B20** табып, Install басыңыз. Немесе GitHub-тан ZIP жүктеп, Sketch, Include Library, Add .ZIP Library.

### Тақталар
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 және Arduino ядросы бар басқалар.

### Қосу
| Сенсор (3 сым) | Uno / Nano | ESP32 |
|---|---|---|
| қызыл (VCC) | 5 В | 3.3 В |
| қара (GND) | GND | GND |
| сары (DATA) | D4 | GPIO4 |

Сары және қызыл сым арасына **4,7 кОм резистор** қою керек. Онсыз сенсор табылмайды. ESP32-де 34-39 пиндерін қолданбаңыз: олар тек кіріс.

### Жылдам бастау
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
- **Паразиттік қуат (екі сым):** `begin()` датчиктердің қуатын өзі біледі және өлшеу кезінде желіні қуатта ұстайды; ұзын желілер үшін `setPowerPin()` (өрістік транзистор кілті) бар.
- **Түсінікті қателер** орысша, қазақша және ағылшынша: `errorText()` және `diagnose(Serial)`.
- **Бірліктер:** `temperature()` °C, `temperatureF()` °F; `temperatureByAddress()` мекенжай бойынша; `setResolution(9..12)` дәлдік пен жылдамдықты өзгертеді.
- **Arduino AVR (Uno, Nano, Mega), ESP32, ESP8266** тақталарында жұмыс істейді. Сыртқы кітапхана қажет емес.

### Әдістер анықтамасы
| Әдіс | Не істейді |
|---|---|
| `begin()` | Желідегі датчиктерді табу, 12 бит қою |
| `temperature(i)` | i-ші датчиктің °C температурасы (қатеде NAN) |
| `temperatureF(i)` | Сол °F-пен |
| `temperatureByAddress(addr)` | 8 байттық мекенжай бойынша датчик |
| `request() / ready()` | Күтусіз өлшеу: іске қосу, содан тексеру |
| `count() / getAddress(i, a)` | Датчик саны және мекенжайлары |
| `setResolution(bits)` | Дәлдік 9-12 бит (жылдамырақ не дәлірек) |
| `rescan()` | Датчиктерді қайта іздеу |
| `setCacheMs(ms)` | Деректер қанша мс жаңа саналады |
| `isParasite() / setPowerMode(mode)` | Датчик қуаты: паразиттік (екі сым) өзі анықталады |
| `setPowerPin(pin, activeHigh)` | Желі қуатының өрістік транзистор кілті |
| `error() / errorText()` | Соңғы қатенің себебі |
| `diagnose(Serial)` | Өзін тексеру |

### Бірдеңе жұмыс істемесе
**Датчик табылмады**

Сигнал сымы (DQ) мен қорек арасына 4,7 кОм резистор керек. GND, VCC және DQ пинін тексеріңіз. Diagnose мысалын іске қосыңыз.

**Температура 85 °C не -127 °C**

85 °C қосылғаннан кейінгі мән, -127 °C сым үзілгенін білдіреді. Өлшеу автоматты қайталанады; болмаса, сымдар мен қоректі тексеріңіз.

**CRC қатесі**

Ұзын сымдар, кедергі не әлсіз қорек. Сымды қысқартыңыз, резисторды датчикке жақын қойыңыз, VCC-ге қорек беріңіз. Паразиттік датчиктерде (екі сым) қуатқа 4,7 кОм резистор қойып, `setPowerPin()` кілтін қосыңыз.

### Өзін-өзі тексеру
Кітапханада `diagnose(Serial)` бар: тақта не көргенін басып шығарады және нені тексеру керегін айтады. Кеңес тілі: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Мысалдар
`Basic`, `Diagnose`, `MultipleSensors`, `NonBlocking`, `ParasitePower`, `Thermostat`.

Толық нұсқаулық: [docs/WIKI.md](docs/WIKI.md).

---

## English

A simple library for **DS18B20** temperature sensors (also DS18S20 and DS1822). Made for classes and Alash Electronics kits: one `begin()` call, several sensors on one wire, clear error messages. **No OneWire or DallasTemperature needed.**

### Installation
Arduino IDE: Sketch, Include Library, Manage Libraries, search **AlashDS18B20** and click Install. Or download the ZIP from GitHub: Sketch, Include Library, Add .ZIP Library.

### Boards
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 and others with an Arduino core.

### Wiring
| Sensor (3 wires) | Uno / Nano | ESP32 |
|---|---|---|
| red (VCC) | 5 V | 3.3 V |
| black (GND) | GND | GND |
| yellow (DATA) | D4 | GPIO4 |

A **4.7 kOhm resistor** between the yellow and red wires is required. On ESP32 avoid pins 34-39: they are input-only.

Parasite power (two wires) is supported: see the `ParasitePower` example. Not supported: alarms and EEPROM copy; use DallasTemperature if you need those.

### Quick start
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
- **Parasite power (two wires):** `begin()` detects how the sensors are powered and holds the line high during each measurement; `setPowerPin()` adds a MOSFET switch for long lines.
- Works on **Arduino AVR (Uno, Nano, Mega), ESP32, ESP8266** and others, with no extra libraries.

### Method reference
| Method | What it does |
|---|---|
| `begin()` | Find the sensors on the line, set 12 bits |
| `temperature(i)` | Temperature in °C of sensor i (NAN on error) |
| `temperatureF(i)` | The same in °F |
| `temperatureByAddress(addr)` | Sensor by its 8-byte address |
| `request() / ready()` | Non-blocking measurement: start, then check |
| `count() / getAddress(i, a)` | Number of sensors and their addresses |
| `setResolution(bits)` | Resolution 9-12 bits (faster or more exact) |
| `rescan()` | Search for sensors again |
| `setCacheMs(ms)` | How many ms data counts as fresh |
| `isParasite() / setPowerMode(mode)` | Sensor power: parasite (two wires) is detected itself; POWER_AUTO / EXTERNAL / PARASITE |
| `setPowerPin(pin, activeHigh)` | MOSFET power switch for the line (many sensors, long wires) |
| `error() / errorText()` | Reason of the last error |
| `diagnose(Serial)` | Self-check |

### If something does not work
**No sensor found**

You need a 4.7 kOhm resistor between the signal wire (DQ) and power. Check GND, VCC and that DQ is on the right pin. Run the Diagnose example.

**Temperature is 85 °C or -127 °C**

85 °C is the power-on value and -127 °C means a broken line. The library repeats the measurement itself; if it still fails, check wires and power.

**CRC error**

Long wires, noise or weak power. Shorten the wire, put the resistor closer to the sensor and power the VCC pin (three wires). With parasite sensors (two wires) add a 4.7 kOhm pull-up and a power switch with `setPowerPin()`.

### Self-check
The library has `diagnose(Serial)`: it prints what the board sees and tells what to check. Tips language: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Examples
`Basic`, `Diagnose`, `MultipleSensors`, `NonBlocking`, `ParasitePower`, `Thermostat`.

Full guide: [docs/WIKI.md](docs/WIKI.md).

---

MIT License, Alash Electronics. https://github.com/Alash-electronics/AlashDS18B20
