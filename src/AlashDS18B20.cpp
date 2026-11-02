// MIT License. Alash Electronics.
#include "AlashDS18B20.h"

// Команды 1-Wire и DS18B20
static const uint8_t CMD_MATCH_ROM = 0x55;
static const uint8_t CMD_SKIP_ROM = 0xCC;
static const uint8_t CMD_CONVERT = 0x44;
static const uint8_t CMD_WRITE_SCRATCHPAD = 0x4E;
static const uint8_t CMD_READ_SCRATCHPAD = 0xBE;
static const uint8_t CMD_READ_POWER = 0xB4;

// Текст на трёх языках: T(по-русски, қазақша, English)
#define T(ru, kk, en) (_lang == LANG_KK ? F(kk) : _lang == LANG_EN ? F(en) : F(ru))

static bool isTempFamily(uint8_t f) {
  return f == AlashDS18B20::FAMILY_DS18B20 || f == AlashDS18B20::FAMILY_DS18S20 || f == AlashDS18B20::FAMILY_DS1822;
}

// ---- Запуск ------------------------------------------------------------------

bool AlashDS18B20::begin() {
  if (rescan() == 0) return false;
  setResolution(12);                       // датчик может помнить другую точность с прошлого раза
  return true;
}

uint8_t AlashDS18B20::rescan() {
  strongPullup(false);
  _count = 0;
  _other = 0;
  _truncated = false;
  _pending = false;
  _hasData = false;
  if (!presence()) return 0;
  _ow.resetSearch();
  uint8_t rom[8];
  while (_ow.search(rom)) {
    if (isTempFamily(rom[0])) {
      if (_count < ALASH_DS18B20_MAX) memcpy(_rom[_count++], rom, 8);
      else _truncated = true;
    } else {
      _other++;
    }
  }
  updateConversionTime();
  if (_count == 0) _err = (_other == 0) ? ERR_CRC : ERR_NO_SENSOR;   // линия ответила, но адресов не получили
  else _err = ERR_OK;
  detectPower();
  return _count;
}

// Определяем питание: датчик с паразитным питанием в ответ на Read Power Supply прижимает линию к земле, с внешним питанием отпускает.
// Если на линии есть хоть один паразитный датчик, считаем паразитной всю линию: так надёжнее.
void AlashDS18B20::detectPower() {
  if (_power == POWER_EXTERNAL) { _parasite = false; return; }
  if (_power == POWER_PARASITE) { _parasite = true; return; }
  _parasite = false;
  if (_count == 0 || !_ow.reset()) return;
  _ow.writeByte(CMD_SKIP_ROM);
  _ow.writeByte(CMD_READ_POWER);
  _parasite = !_ow.readBit();
}

void AlashDS18B20::setPowerMode(PowerMode mode) {
  _power = mode;
  detectPower();
}

void AlashDS18B20::setPowerPin(int8_t pin, bool activeHigh) {
  _powerPin = pin;
  _powerActiveHigh = activeHigh;
  if (pin >= 0) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, activeHigh ? LOW : HIGH);   // ключ выключен
  }
}

void AlashDS18B20::strongPullup(bool on) {
  if (on == _strong) return;
  _strong = on;
  if (_powerPin >= 0) digitalWrite(_powerPin, (on == _powerActiveHigh) ? HIGH : LOW);
  else if (on) _ow.powerOn();
  else _ow.powerOff();
}

bool AlashDS18B20::presence() {
  strongPullup(false);                     // перед любым обменом линию надо отпустить
  if (_ow.reset()) return true;
  _err = _ow.lineHigh() ? ERR_NO_SENSOR : ERR_SHORT;
  return false;
}

void AlashDS18B20::select(uint8_t index) {
  if (_count == 1 && _other == 0) {
    _ow.writeByte(CMD_SKIP_ROM);
  } else {
    _ow.writeByte(CMD_MATCH_ROM);
    for (uint8_t i = 0; i < 8; i++) _ow.writeByte(_rom[index][i]);
  }
}

// ---- Измерения -----------------------------------------------------------------

bool AlashDS18B20::request() {
  if (_count == 0) {
    _err = ERR_NO_SENSOR;
    return false;
  }
  if (!presence()) return false;
  _ow.writeByte(CMD_SKIP_ROM);             // все датчики измеряют одновременно
  _ow.writeByte(CMD_CONVERT);
  if (_parasite) strongPullup(true);       // питание сразу после команды (не позже 10 мкс) и на всё время измерения
  _pending = true;
  _reqAt = millis();
  return true;
}

bool AlashDS18B20::ready() {
  bool done = _pending && (uint32_t)(millis() - _reqAt) >= _convMs;
  if (done) strongPullup(false);           // измерение закончилось: линию можно отпустить
  return done;
}

bool AlashDS18B20::ensureFresh() {
  if (_count == 0) {
    _err = ERR_NO_SENSOR;
    return false;
  }
  if (!_pending) {
    if (_hasData && (uint32_t)(millis() - _lastRead) < _cacheMs) return true;
    if (!request()) return false;
  }
  uint32_t elapsed = millis() - _reqAt;
  if (elapsed < _convMs) delay(_convMs - elapsed);
  strongPullup(false);
  readAll();

  // 85,0 °C это значение датчика после включения. Измеряем ещё раз, чтобы не принять его за температуру.
  bool again = false;
  for (uint8_t i = 0; i < _count; i++)
    if (_errs[i] == ERR_OK && _t[i] == 85.0f) again = true;
  if (again && request()) {
    delay(_convMs);
    strongPullup(false);
    readAll();
  }
  return true;
}

bool AlashDS18B20::update() {
  strongPullup(false);
  _pending = false;
  _hasData = false;
  return ensureFresh();
}

float AlashDS18B20::temperature(uint8_t index) {
  if (index >= _count) {
    _err = (_count == 0) ? ERR_NO_SENSOR : ERR_NO_SUCH;
    return NAN;
  }
  if (!ensureFresh()) return NAN;
  _err = _errs[index];
  return _errs[index] == ERR_OK ? _t[index] : NAN;
}

float AlashDS18B20::temperatureF(uint8_t index) {
  float c = temperature(index);
  return isnan(c) ? NAN : c * 1.8f + 32.0f;
}

float AlashDS18B20::temperatureByAddress(const uint8_t *address) {
  for (uint8_t i = 0; i < _count; i++)
    if (memcmp(_rom[i], address, 8) == 0) return temperature(i);
  _err = ERR_NO_SUCH;
  return NAN;
}

// Читает «блокнот» датчика (9 байт) и проверяет CRC. До трёх попыток при ошибке.
bool AlashDS18B20::readScratchpad(uint8_t index, uint8_t *sp) {
  for (uint8_t attempt = 0; attempt < 3; attempt++) {
    if (!presence()) return false;
    select(index);
    _ow.writeByte(CMD_READ_SCRATCHPAD);
    bool allFF = true;
    for (uint8_t i = 0; i < 9; i++) {
      sp[i] = _ow.readByte();
      if (sp[i] != 0xFF) allFF = false;
    }
    if (allFF) {                           // линия подтянута, но датчик молчит
      _err = ERR_NO_SENSOR;
      return false;
    }
    if (AlashOneWire::crc8(sp, 8) == sp[8]) return true;
    _err = ERR_CRC;
  }
  return false;
}

float AlashDS18B20::decode(uint8_t family, const uint8_t *sp) const {
  int16_t raw = (int16_t)(((uint16_t)sp[1] << 8) | sp[0]);
  if (family == FAMILY_DS18S20) {
    raw &= ~1;                             // шаг 0,5 °C; точнее считаем по остатку счётчика
    float t = raw / 2.0f;
    if (sp[7] != 0) t = (float)(raw >> 1) - 0.25f + (float)(sp[7] - sp[6]) / sp[7];
    return t;
  }
  uint8_t bits = ((sp[4] >> 5) & 3) + 9;   // точность записана в регистре настройки
  if (bits == 9) raw &= ~7;                // младшие биты при малой точности не определены
  else if (bits == 10) raw &= ~3;
  else if (bits == 11) raw &= ~1;
  return raw / 16.0f;
}

bool AlashDS18B20::readAll() {
  uint8_t sp[9];
  _pending = false;
  bool any = false;
  for (uint8_t i = 0; i < _count; i++) {
    if (readScratchpad(i, sp)) {
      _t[i] = decode(_rom[i][0], sp);
      _errs[i] = ERR_OK;
      any = true;
    } else {
      _t[i] = NAN;
      _errs[i] = _err;
    }
  }
  _lastRead = millis();
  _hasData = true;
  return any;
}

// ---- Датчики и настройки -------------------------------------------------------------

bool AlashDS18B20::getAddress(uint8_t index, uint8_t *address8) {
  if (index >= _count) {
    _err = ERR_NO_SUCH;
    return false;
  }
  memcpy(address8, _rom[index], 8);
  return true;
}

void AlashDS18B20::printAddress(Print &out, uint8_t index) {
  if (index >= _count) return;
  for (uint8_t i = 0; i < 8; i++) {
    if (_rom[index][i] < 16) out.print('0');
    out.print(_rom[index][i], HEX);
  }
}

const __FlashStringHelper *AlashDS18B20::modelName(uint8_t index) const {
  switch (family(index)) {
    case FAMILY_DS18B20: return F("DS18B20");
    case FAMILY_DS18S20: return F("DS18S20");
    case FAMILY_DS1822: return F("DS1822");
  }
  return F("?");
}

void AlashDS18B20::updateConversionTime() {
  bool s20 = false;
  for (uint8_t i = 0; i < _count; i++)
    if (_rom[i][0] == FAMILY_DS18S20) s20 = true;
  if (s20 || _res >= 12) _convMs = 750;    // DS18S20 всегда 750 мс
  else if (_res == 11) _convMs = 375;
  else if (_res == 10) _convMs = 188;
  else _convMs = 94;
}

bool AlashDS18B20::setResolution(uint8_t bits) {
  if (bits < 9 || bits > 12) return false;
  _res = bits;
  updateConversionTime();
  _pending = false;
  _hasData = false;
  if (_count == 0) return false;
  if (!presence()) return false;
  _ow.writeByte(CMD_SKIP_ROM);
  _ow.writeByte(CMD_WRITE_SCRATCHPAD);
  _ow.writeByte(0x4B);                     // TH и TL оставляем заводскими, будильник не используем
  _ow.writeByte(0x46);
  _ow.writeByte(((bits - 9) << 5) | 0x1F); // регистр настройки: R1 R0 + служебные единицы
  return true;
}

// ---- Диагностика ------------------------------------------------------------------------

const __FlashStringHelper *AlashDS18B20::errorText() {
  switch (_err) {
    case ERR_OK: return F("OK");
    case ERR_NO_SENSOR:
      return T("Датчик не найден: проверьте провода (красный 3.3-5 В, чёрный GND, жёлтый на пин) и резистор 4.7 кОм между данными и питанием",
               "Сенсор табылмады: сымдарды (қызыл 3.3-5 В, қара GND, сары пинге) және дерек пен қуат арасындағы 4.7 кОм резисторды тексеріңіз",
               "Sensor not found: check the wires (red 3.3-5 V, black GND, yellow to the pin) and the 4.7 kOhm resistor between data and power");
    case ERR_SHORT:
      return T("Линия данных замкнута на землю или нет резистора 4.7 кОм между данными и питанием",
               "Дерек желісі жерге тұйықталған немесе дерек пен қуат арасында 4.7 кОм резисторы жоқ",
               "The data line is shorted to GND or the 4.7 kOhm pull-up resistor is missing");
    case ERR_CRC:
      return T("Данные пришли с ошибкой: укоротите провода, проверьте питание и резистор 4.7 кОм",
               "Деректер қатемен келді: сымдарды қысқартыңыз, қуатты және 4.7 кОм резисторды тексеріңіз",
               "Data arrived with errors: use shorter wires, check power and the 4.7 kOhm resistor");
    case ERR_NO_SUCH:
      return T("Датчика с таким номером нет (номера начинаются с 0, всего см. count())",
               "Мұндай нөмірлі сенсор жоқ (нөмір 0-ден басталады, санын count() көрсетеді)",
               "There is no sensor with that number (numbers start at 0, see count())");
  }
  return F("");
}

void AlashDS18B20::diagnose(Print &out) {
  out.println(T("AlashDS18B20: диагностика", "AlashDS18B20: диагностика", "AlashDS18B20: diagnostics"));
  out.print(T("Пин данных: ", "Дерек пині: ", "Data pin: "));
  out.println(_pin);

  bool high = _ow.lineHigh();
  out.print(T("Линия: ", "Желі: ", "Line: "));
  out.println(high ? T("подтянута к питанию, это хорошо", "қуатқа тартылған, бұл жақсы", "pulled up, good")
                   : T("прижата к земле или нет резистора 4.7 кОм", "жерге басылған немесе 4.7 кОм резисторы жоқ", "stuck low or no 4.7 kOhm pull-up"));

  rescan();
  out.print(T("Найдено датчиков температуры: ", "Табылған температура сенсорлары: ", "Temperature sensors found: "));
  out.println(_count);
  for (uint8_t i = 0; i < _count; i++) {
    out.print(F("  #"));
    out.print(i);
    out.print(F("  "));
    printAddress(out, i);
    out.print(F("  "));
    out.print(modelName(i));
    out.print(F("  "));
    float t = temperature(i);
    if (isnan(t)) out.println(errorText());
    else {
      out.print(t, 2);
      out.println(F(" C"));
    }
  }
  if (_count) {
    out.print(T("Питание датчиков: ", "Сенсорлардың қуаты: ", "Sensor power: "));
    out.println(_parasite ? T("паразитное (два провода)", "паразиттік (екі сым)", "parasite (two wires)")
                          : T("внешнее (три провода)", "сыртқы (үш сым)", "external (three wires)"));
  }
  if (_other) {
    out.print(T("Другие устройства 1-Wire: ", "Басқа 1-Wire құрылғылары: ", "Other 1-Wire devices: "));
    out.println(_other);
  }
  if (_truncated) out.println(T("Датчиков больше, чем ALASH_DS18B20_MAX: лишние пропущены", "Сенсорлар ALASH_DS18B20_MAX-тан көп: артығы өткізілді", "More sensors than ALASH_DS18B20_MAX: the extra ones are skipped"));
  if (_count == 0) {
    out.println(errorText());
  } else {
    for (uint8_t i = 0; i < _count; i++)
      if (_errs[i] == ERR_OK && _t[i] == 85.0f) {
        out.println(T("Показывает ровно 85 C: так датчик отвечает без нормального питания. Проверьте 3.3-5 В и GND",
                      "Дәл 85 C көрсетеді: сенсор қуатсыз осылай жауап береді. 3.3-5 В және GND тексеріңіз",
                      "Reads exactly 85 C: that is the power-on value. Check the 3.3-5 V and GND wires"));
        if (_parasite) out.println(T("При паразитном питании это ещё и признак слабого питания во время измерения: поставьте резистор 4,7 кОм на питание, укоротите линию или подключите ключ питания setPowerPin().",
                                     "Паразиттік қуатта бұл өлшеу кезіндегі әлсіз қуаттың белгісі: қуатқа 4,7 кОм резистор қойыңыз, желіні қысқартыңыз не setPowerPin() қуат кілтін қосыңыз.",
                                     "With parasite power this also means weak power during the measurement: add a 4.7 kOhm pull-up, shorten the line or add a power switch with setPowerPin()."));
        break;
      }
  }
}
