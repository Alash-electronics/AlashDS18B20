// Минимальная имитация Arduino.h для запуска тестов на компьютере (g++).
#pragma once
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
using __FlashStringHelper = char;
#define F(x) ((const __FlashStringHelper *)(x))
#define HEX 16
#define DEC 10
#define INPUT 0
#define INPUT_PULLUP 2
#define OUTPUT 1
#define LOW 0
#define HIGH 1
inline uint32_t g_now = 0;
inline uint32_t millis() { return g_now; }
inline void delay(uint32_t ms) { g_now += ms; }
inline void delayMicroseconds(uint32_t) {}
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
inline void noInterrupts() {}
inline void interrupts() {}
class Print {
 public:
  virtual ~Print() {}
  virtual void print(const char *s) { printf("%s", s); }
  virtual void print(char c) { printf("%c", c); }
  virtual void print(unsigned v, int base = DEC) { printf(base == HEX ? "%X" : "%u", v); }
  virtual void print(int v, int base = DEC) { printf(base == HEX ? "%X" : "%d", v); }
  virtual void print(float v, int digits = 2) { printf("%.*f", digits, v); }
  virtual void println(const char *s = "") { printf("%s\n", s); }
  virtual void println(unsigned v, int base = DEC) { print(v, base); printf("\n"); }
  virtual void println(int v, int base = DEC) { print(v, base); printf("\n"); }
};
