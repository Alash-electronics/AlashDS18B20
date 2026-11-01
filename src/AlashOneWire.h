// AlashOneWire: минимальный слой протокола 1-Wire (сброс, биты, байты, поиск устройств, CRC8).
// Внутренний класс AlashDS18B20, подключать его самому не нужно.
// MIT License. Alash Electronics.
#ifndef ALASH_ONEWIRE_H
#define ALASH_ONEWIRE_H

#include <Arduino.h>

class AlashOneWire {
 public:
  explicit AlashOneWire(uint8_t pin = 0) { setPin(pin); }
  void setPin(uint8_t pin);
  void setPullup(bool on);

  bool lineHigh();            // линия подтянута к питанию (не замкнута на землю)
  bool reset();               // импульс сброса; true, если хоть одно устройство ответило
  void writeByte(uint8_t v);
  uint8_t readByte();
  bool readBit();
  void writeBit(bool bit);

  // Поиск устройств на линии (Maxim AN187). Первый вызов: resetSearch(), затем search() пока true.
  void resetSearch();
  bool search(uint8_t *rom);

  static uint8_t crc8(const uint8_t *data, uint8_t len);

 private:
  uint8_t _pin = 0;
  bool _pullup = false;
  uint8_t _lastDiscrepancy = 0;
  bool _lastDevice = false;
  uint8_t _lastRom[8] = {0};
#if defined(ARDUINO_ARCH_AVR)
  volatile uint8_t *_ddr = nullptr, *_port = nullptr, *_pin_reg = nullptr;
  uint8_t _mask = 0;
#endif
  void release();             // отпустить линию (вход)
  void pullLow();             // прижать линию к земле
  bool sample();              // прочитать уровень
};

#endif  // ALASH_ONEWIRE_H
