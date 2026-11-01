// Имитация линии 1-Wire с датчиками DS18B20 на уровне битов (поиск ROM, Match/Skip ROM, измерение, чтение блокнота).
// Подменяет нижний слой AlashOneWire (сборка с -DALASH_OW_EXTERNAL).
#pragma once
#include <math.h>
#include <string.h>
#include <vector>
#include "AlashOneWire.h"

struct SimDev {
  uint8_t rom[8];
  uint8_t sp[9];
  float temp = 20.0f;
  bool first85 = false;      // первое измерение даёт 85 °C (как после включения)
  bool corrupt = false;      // портит CRC блокнота
  bool converted = false;
  int convertCount = 0;
  uint8_t cfg = 0x7F;        // регистр настройки (12 бит)
  bool sel = false;          // выбран мастером
  bool part = false;         // участвует в поиске
};

struct SimBus {
  std::vector<SimDev> devs;
  bool shorted = false;
  enum State { ROM_CMD, SEARCH, MATCH, FUNC_CMD, READ_SP, WRITE_SP, DEAD } st = ROM_CMD;
  int cmdBits = 0; uint8_t cmd = 0;
  int bitpos = 0, phase = 0; uint8_t matchBuf[8]; int writeBytes = 0;
  int spBit = 0; uint8_t wbuf[3];
  long resets = 0;

  void addDevice(uint8_t family, uint8_t b1, float t) {
    SimDev d;
    d.rom[0] = family; d.rom[1] = b1;
    for (int i = 2; i < 7; i++) d.rom[i] = (uint8_t)(b1 * 7 + i * 13 + family);
    d.rom[7] = AlashOneWire::crc8(d.rom, 7);
    d.temp = t;
    memset(d.sp, 0, sizeof d.sp);
    d.sp[0] = 0x50; d.sp[1] = 0x05;      // 85 °C после включения
    d.sp[2] = 0x4B; d.sp[3] = 0x46; d.sp[4] = 0x7F; d.sp[5] = 0xFF; d.sp[6] = 0x0C; d.sp[7] = 0x10;
    devs.push_back(d);
  }
  static uint8_t crcSp(const uint8_t *sp) { return AlashOneWire::crc8(sp, 8); }

  void convert(SimDev &d) {
    d.convertCount++;
    float t = (d.first85 && d.convertCount == 1) ? 85.0f : d.temp;
    if (d.rom[0] == 0x10) {                // DS18S20: шаг 0,5 °C и счётчик остатка
      float fl = floorf(t * 2.0f) / 2.0f;
      int16_t raw = (int16_t)lroundf(fl * 2.0f);
      d.sp[0] = raw & 0xFF; d.sp[1] = (raw >> 8) & 0xFF;
      d.sp[6] = 0x0C; d.sp[7] = 0x10;
    } else {
      int16_t raw = (int16_t)lroundf(t * 16.0f);
      uint8_t res = ((d.cfg >> 5) & 3);
      if (res == 0) raw &= ~7; else if (res == 1) raw &= ~3; else if (res == 2) raw &= ~1;
      d.sp[0] = raw & 0xFF; d.sp[1] = (raw >> 8) & 0xFF;
      d.sp[4] = d.cfg;
    }
    d.sp[8] = crcSp(d.sp);
    if (d.corrupt) d.sp[8] ^= 0x55;
  }

  bool reset() {
    resets++;
    if (shorted || devs.empty()) return false;
    st = ROM_CMD; cmdBits = 0; cmd = 0;
    for (auto &d : devs) { d.sel = false; d.part = true; }
    return true;
  }

  void writeBit(bool b) {
    switch (st) {
      case ROM_CMD:
      case FUNC_CMD:
        cmd |= (b ? 1 : 0) << cmdBits;
        if (++cmdBits == 8) { handle(cmd); cmd = 0; cmdBits = 0; }
        break;
      case SEARCH:                          // мастер выбирает направление, неподходящие устройства замолкают
        for (auto &d : devs)
          if (d.part && (((d.rom[bitpos / 8] >> (bitpos % 8)) & 1) != (b ? 1 : 0))) d.part = false;
        bitpos++; phase = 0;
        if (bitpos == 64) st = DEAD;
        break;
      case MATCH:
        if (b) matchBuf[bitpos / 8] |= 1 << (bitpos % 8);
        if (++bitpos == 64) {
          for (auto &d : devs) d.sel = (memcmp(d.rom, matchBuf, 8) == 0);
          st = FUNC_CMD; cmd = 0; cmdBits = 0;
        }
        break;
      case WRITE_SP:
        if (b) wbuf[writeBytes] |= 1 << cmdBits;
        if (++cmdBits == 8) {
          cmdBits = 0;
          if (++writeBytes == 3) {
            for (auto &d : devs) if (d.sel) { d.sp[2] = wbuf[0]; d.sp[3] = wbuf[1]; d.cfg = wbuf[2]; d.sp[4] = wbuf[2]; }
            st = DEAD;
          }
        }
        break;
      default: break;
    }
  }

  void handle(uint8_t c) {
    if (st == ROM_CMD) {
      if (c == 0xF0) { st = SEARCH; bitpos = 0; phase = 0; }
      else if (c == 0xCC) { for (auto &d : devs) d.sel = true; st = FUNC_CMD; }
      else if (c == 0x55) { st = MATCH; bitpos = 0; memset(matchBuf, 0, 8); }
      else st = DEAD;
    } else {                                // функциональная команда
      if (c == 0x44) { for (auto &d : devs) if (d.sel) convert(d); st = DEAD; }
      else if (c == 0xBE) { st = READ_SP; spBit = 0; }
      else if (c == 0x4E) { st = WRITE_SP; writeBytes = 0; cmdBits = 0; memset(wbuf, 0, 3); }
      else st = DEAD;
    }
  }

  bool readBit() {
    if (st == SEARCH) {
      bool any = false, bit = true, cmp = true;
      for (auto &d : devs) if (d.part) {
        any = true;
        bool rb = (d.rom[bitpos / 8] >> (bitpos % 8)) & 1;
        bit = bit && rb;                    // линия: «0» побеждает (проводное И)
        cmp = cmp && !rb;
      }
      if (!any) return true;
      return phase++ == 0 ? bit : cmp;
    }
    if (st == READ_SP) {
      bool v = true;
      for (auto &d : devs) if (d.sel) v = v && ((d.sp[spBit / 8] >> (spBit % 8)) & 1);
      if (++spBit == 72) st = DEAD;
      return v;
    }
    return true;
  }
};

inline SimBus g_bus;

void AlashOneWire::setPin(uint8_t pin) { _pin = pin; }
void AlashOneWire::setPullup(bool on) { _pullup = on; }
bool AlashOneWire::lineHigh() { return !g_bus.shorted; }
bool AlashOneWire::reset() { return g_bus.reset(); }
void AlashOneWire::writeBit(bool b) { g_bus.writeBit(b); }
bool AlashOneWire::readBit() { return g_bus.readBit(); }
