// AlashDS18B20: датчики температуры DS18B20 (и DS18S20, DS1822) на линии 1-Wire.
// Без зависимостей: не нужны библиотеки OneWire и DallasTemperature.
// MIT License. Alash Electronics, https://github.com/Alash-electronics/AlashDS18B20
#ifndef ALASH_DS18B20_H
#define ALASH_DS18B20_H

#include <Arduino.h>
#include "AlashOneWire.h"

// Сколько датчиков библиотека помнит на одной линии (можно задать до #include: #define ALASH_DS18B20_MAX 4)
#ifndef ALASH_DS18B20_MAX
#if defined(ARDUINO_ARCH_AVR)
#define ALASH_DS18B20_MAX 8
#else
#define ALASH_DS18B20_MAX 16
#endif
#endif

class AlashDS18B20 {
 public:
  // Что пошло не так (для errorText() и diagnose())
  enum Error : uint8_t {
    ERR_OK = 0,
    ERR_NO_SENSOR,   // на линии никто не отвечает
    ERR_SHORT,       // линия прижата к земле или нет подтягивающего резистора
    ERR_CRC,         // данные пришли с ошибкой (длинные провода, помехи, слабое питание)
    ERR_NO_SUCH      // датчика с таким номером нет
  };

  // Язык подсказок errorText() и diagnose()
  enum Language : uint8_t { LANG_RU = 0, LANG_KK = 1, LANG_EN = 2 };

  // Семейства микросхем (первый байт адреса)
  enum Family : uint8_t { FAMILY_DS18S20 = 0x10, FAMILY_DS1822 = 0x22, FAMILY_DS18B20 = 0x28 };

  explicit AlashDS18B20(uint8_t pin) : _pin(pin), _ow(pin) {}

  // ---- Запуск --------------------------------------------------------------
  // begin() ищет все датчики на линии и ставит точность 12 бит. true, если найден хотя бы один.
  bool begin();
  uint8_t rescan();                        // искать датчики заново (например, после подключения нового)
  void setPullup(bool on) { _ow.setPullup(on); }   // внутренняя подтяжка МК (лучше внешний резистор 4,7 кОм)

  // ---- Измерения (при первом вызове за цикл датчики измеряют сами) -------------
  float temperature(uint8_t index = 0);    // °C; NAN, если не получилось (причина в error())
  float temperatureF(uint8_t index = 0);   // °F
  float temperatureByAddress(const uint8_t *address);   // датчик по 8-байтовому адресу
  bool update();                           // принудительно измерить все датчики (ждёт окончания)

  // ---- Без блокировки: запустить измерение и забрать результат позже ----------
  bool request();                          // запустить измерение на всех датчиках сразу
  bool ready();                            // true, когда измерение закончено (можно читать temperature())

  // ---- Датчики --------------------------------------------------------------
  uint8_t count() const { return _count; }
  bool getAddress(uint8_t index, uint8_t *address8);
  void printAddress(Print &out, uint8_t index);         // адрес в виде 28FF1A2B3C4D5E6F
  uint8_t family(uint8_t index) const { return index < _count ? _rom[index][0] : 0; }
  const __FlashStringHelper *modelName(uint8_t index) const;

  // ---- Настройки -----------------------------------------------------------
  bool setResolution(uint8_t bits);        // 9, 10, 11 или 12 бит (шаг 0,5 / 0,25 / 0,125 / 0,0625 °C)
  uint8_t resolution() const { return _res; }
  uint16_t conversionTimeMs() const { return _convMs; }
  void setCacheMs(uint16_t ms) { _cacheMs = ms; }       // сколько мс считаются «свежими» последние данные (по умолчанию 500)
  void setLanguage(Language lang) { _lang = lang; }

  // ---- Диагностика -----------------------------------------------------------
  Error error() const { return _err; }
  const __FlashStringHelper *errorText();               // понятное описание последней ошибки (текст во флеш-памяти)
  void diagnose(Print &out);                            // печатает линию, найденные датчики и подсказки

 private:
  uint8_t _pin;
  AlashOneWire _ow;
  uint8_t _count = 0;
  uint8_t _other = 0;                      // посторонние устройства 1-Wire на линии
  bool _truncated = false;                 // датчиков больше, чем ALASH_DS18B20_MAX
  uint8_t _rom[ALASH_DS18B20_MAX][8];
  float _t[ALASH_DS18B20_MAX];
  Error _errs[ALASH_DS18B20_MAX];
  Error _err = ERR_OK;
  Language _lang = LANG_RU;
  uint8_t _res = 12;
  uint16_t _convMs = 750;
  uint16_t _cacheMs = 500;
  bool _pending = false;                   // измерение запущено, результат ещё не прочитан
  bool _hasData = false;
  uint32_t _reqAt = 0;
  uint32_t _lastRead = 0;

  bool presence();                         // сброс линии; при неудаче ставит ERR_SHORT или ERR_NO_SENSOR
  void select(uint8_t index);              // Skip ROM (один датчик) или Match ROM
  bool readScratchpad(uint8_t index, uint8_t *sp);
  float decode(uint8_t family, const uint8_t *sp) const;
  bool ensureFresh();
  bool readAll();
  void updateConversionTime();
};

#endif  // ALASH_DS18B20_H
