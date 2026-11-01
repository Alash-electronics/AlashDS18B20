// MIT License. Alash Electronics.
#include "AlashOneWire.h"

// Для тестов без железа линию подменяет файл extras/test (ALASH_OW_EXTERNAL): reset, readBit, writeBit, lineHigh, setPin, setPullup.
#ifndef ALASH_OW_EXTERNAL

void AlashOneWire::setPin(uint8_t pin) {
  _pin = pin;
#if defined(ARDUINO_ARCH_AVR)
  _mask = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);
  _ddr = portModeRegister(port);
  _port = portOutputRegister(port);
  _pin_reg = portInputRegister(port);
#endif
  release();
}

void AlashOneWire::setPullup(bool on) {
  _pullup = on;
  release();
}

// ---- Работа с ножкой: на AVR напрямую через регистры (быстро), на остальных через pinMode ----
#if defined(ARDUINO_ARCH_AVR)
void AlashOneWire::release() {
  *_ddr &= ~_mask;
  if (_pullup) *_port |= _mask; else *_port &= ~_mask;
}
void AlashOneWire::pullLow() {
  *_port &= ~_mask;
  *_ddr |= _mask;
}
bool AlashOneWire::sample() { return (*_pin_reg & _mask) != 0; }
#else
void AlashOneWire::release() { pinMode(_pin, _pullup ? INPUT_PULLUP : INPUT); }
void AlashOneWire::pullLow() {
  digitalWrite(_pin, LOW);
  pinMode(_pin, OUTPUT);
}
bool AlashOneWire::sample() { return digitalRead(_pin) != 0; }
#endif

bool AlashOneWire::lineHigh() {
  release();
  for (uint8_t i = 0; i < 125; i++) {      // линия должна подняться за ~250 мкс
    if (sample()) return true;
    delayMicroseconds(2);
  }
  return false;
}

bool AlashOneWire::reset() {
  if (!lineHigh()) return false;           // линия замкнута на землю или нет подтяжки
  noInterrupts();
  pullLow();
  interrupts();
  delayMicroseconds(480);
  noInterrupts();
  release();
  delayMicroseconds(70);
  bool present = !sample();                // устройство отвечает, прижимая линию
  interrupts();
  delayMicroseconds(410);
  return present;
}

void AlashOneWire::writeBit(bool bit) {
  noInterrupts();
  pullLow();
  if (bit) {
    delayMicroseconds(10);
    release();
    interrupts();
    delayMicroseconds(55);
  } else {
    delayMicroseconds(65);
    release();
    interrupts();
    delayMicroseconds(5);
  }
}

bool AlashOneWire::readBit() {
  noInterrupts();
  pullLow();
  delayMicroseconds(3);
  release();
  delayMicroseconds(10);
  bool v = sample();
  interrupts();
  delayMicroseconds(53);
  return v;
}

#endif  // ALASH_OW_EXTERNAL

void AlashOneWire::writeByte(uint8_t v) {
  for (uint8_t mask = 1; mask; mask <<= 1) writeBit(v & mask);
}

uint8_t AlashOneWire::readByte() {
  uint8_t v = 0;
  for (uint8_t mask = 1; mask; mask <<= 1)
    if (readBit()) v |= mask;
  return v;
}

void AlashOneWire::resetSearch() {
  _lastDiscrepancy = 0;
  _lastDevice = false;
  memset(_lastRom, 0, sizeof _lastRom);
}

// Алгоритм поиска ROM по Maxim AN187: на каждом бите устройства отвечают битом и его инверсией.
bool AlashOneWire::search(uint8_t *rom) {
  if (_lastDevice) return false;
  if (!reset()) {
    resetSearch();
    return false;
  }
  writeByte(0xF0);                          // Search ROM

  uint8_t bitNum = 1, lastZero = 0, idx = 0, mask = 1;
  while (idx < 8) {
    bool idBit = readBit();
    bool cmpBit = readBit();
    if (idBit && cmpBit) break;             // никто не отвечает
    bool dir;
    if (idBit != cmpBit) {
      dir = idBit;                          // у всех оставшихся здесь одинаковый бит
    } else {                                // конфликт: на этом бите устройства различаются
      if (bitNum < _lastDiscrepancy) dir = (_lastRom[idx] & mask) != 0;
      else dir = (bitNum == _lastDiscrepancy);
      if (!dir) lastZero = bitNum;
    }
    if (dir) _lastRom[idx] |= mask; else _lastRom[idx] &= ~mask;
    writeBit(dir);
    bitNum++;
    mask <<= 1;
    if (mask == 0) {
      idx++;
      mask = 1;
    }
  }

  if (idx == 8 && _lastRom[0] != 0 && crc8(_lastRom, 7) == _lastRom[7]) {
    _lastDiscrepancy = lastZero;
    if (lastZero == 0) _lastDevice = true;
    memcpy(rom, _lastRom, 8);
    return true;
  }
  resetSearch();
  return false;
}

// CRC8 Dallas/Maxim (полином x^8 + x^5 + x^4 + 1), побитово, чтобы не занимать память таблицей.
uint8_t AlashOneWire::crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  while (len--) {
    uint8_t b = *data++;
    for (uint8_t i = 0; i < 8; i++) {
      uint8_t mix = (crc ^ b) & 1;
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      b >>= 1;
    }
  }
  return crc;
}
