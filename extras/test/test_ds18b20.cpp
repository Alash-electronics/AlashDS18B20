// Тесты AlashDS18B20 без железа (имитация линии 1-Wire):
//   g++ -std=c++17 -DALASH_OW_EXTERNAL -DALASH_DS18B20_MAX=4 -I mock -I ../../src test_ds18b20.cpp ../../src/AlashDS18B20.cpp ../../src/AlashOneWire.cpp && ./a.out
#include <assert.h>
#include <stdio.h>
#include "Arduino.h"
#include "AlashDS18B20.h"
#include "Bus.h"

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); fails++; } } while (0)
static bool near(float a, float b, float eps) { return fabsf(a - b) <= eps; }

static void fresh() { g_bus = SimBus(); g_now = 1000; }

int main() {
  // 1) CRC8: пример из Maxim AN27: 02 1C B8 01 00 00 00 -> A2
  const uint8_t v[] = {0x02, 0x1C, 0xB8, 0x01, 0x00, 0x00, 0x00};
  CHECK(AlashOneWire::crc8(v, 7) == 0xA2);

  // 2) один датчик: Skip ROM, температура 23.5 °C, °F
  fresh();
  g_bus.addDevice(0x28, 1, 23.5f);
  AlashDS18B20 s(4);
  CHECK(s.begin());
  CHECK(s.count() == 1);
  CHECK(s.family(0) == AlashDS18B20::FAMILY_DS18B20);
  CHECK(near(s.temperature(), 23.5f, 0.001f));
  CHECK(s.error() == AlashDS18B20::ERR_OK);
  CHECK(near(s.temperatureF(), 74.3f, 0.01f));

  // 3) отрицательная температура и шаг 0,0625
  fresh();
  g_bus.addDevice(0x28, 1, -10.125f);
  AlashDS18B20 neg(4);
  CHECK(neg.begin());
  CHECK(near(neg.temperature(), -10.125f, 0.001f));

  // 4) три датчика: поиск находит все, у каждого своя температура, обращение по номеру и по адресу
  fresh();
  g_bus.addDevice(0x28, 10, 20.0f);
  g_bus.addDevice(0x28, 11, 30.5f);
  g_bus.addDevice(0x28, 12, -5.0f);
  AlashDS18B20 m(4);
  CHECK(m.begin());
  CHECK(m.count() == 3);
  uint8_t a0[8], a1[8], a2[8];
  CHECK(m.getAddress(0, a0) && m.getAddress(1, a1) && m.getAddress(2, a2));
  CHECK(memcmp(a0, a1, 8) != 0 && memcmp(a1, a2, 8) != 0 && memcmp(a0, a2, 8) != 0);
  float sum = m.temperature(0) + m.temperature(1) + m.temperature(2);
  CHECK(near(sum, 45.5f, 0.001f));
  CHECK(near(m.temperatureByAddress(a0), m.temperature(0), 0.0001f));
  // каждый датчик отдаёт именно свою температуру (порядок задаёт поиск, поэтому сверяем по адресу)
  for (int i = 0; i < 3; i++) {
    uint8_t ad[8]; m.getAddress(i, ad);
    float expect = ad[1] == 10 ? 20.0f : ad[1] == 11 ? 30.5f : -5.0f;
    CHECK(near(m.temperature(i), expect, 0.001f));
  }

  // 5) индекс вне диапазона
  CHECK(isnan(m.temperature(7)));
  CHECK(m.error() == AlashDS18B20::ERR_NO_SUCH);

  // 6) кэш: второй вызов сразу не ждёт новое измерение
  fresh();
  g_bus.addDevice(0x28, 1, 21.0f);
  AlashDS18B20 c(4);
  c.begin();
  c.temperature();
  uint32_t t1 = g_now;
  c.temperature();
  CHECK(g_now == t1);
  g_now += 600;                                 // кэш (500 мс) устарел
  c.temperature();
  CHECK(g_now >= t1 + 600 + 750);

  // 7) без блокировки: request() -> ready() -> temperature() без ожидания
  fresh();
  g_bus.addDevice(0x28, 1, 22.0f);
  AlashDS18B20 nb(4);
  nb.begin();
  CHECK(nb.request());
  CHECK(!nb.ready());
  g_now += 800;
  CHECK(nb.ready());
  uint32_t t2 = g_now;
  CHECK(near(nb.temperature(), 22.0f, 0.001f));
  CHECK(g_now == t2);
  CHECK(!nb.ready());

  // 8) точность 9 бит: время измерения 94 мс, температура округлена до 0,5
  fresh();
  g_bus.addDevice(0x28, 1, 23.4375f);
  AlashDS18B20 r(4);
  r.begin();
  CHECK(r.setResolution(9));
  CHECK(r.conversionTimeMs() == 94);
  CHECK(r.resolution() == 9);
  CHECK(near(r.temperature(), 23.0f, 0.001f));
  CHECK(r.setResolution(12));
  CHECK(!r.setResolution(8));
  CHECK(!r.setResolution(13));
  CHECK(near(r.temperature(), 23.4375f, 0.001f));

  // 9) DS18S20: расчёт по счётчику остатка (25.0 °C)
  fresh();
  g_bus.addDevice(0x10, 1, 25.0f);
  AlashDS18B20 old(4);
  CHECK(old.begin());
  CHECK(near(old.temperature(), 25.0f, 0.01f));
  CHECK(old.conversionTimeMs() == 750);

  // 10) 85 °C после включения: библиотека измеряет ещё раз
  fresh();
  g_bus.addDevice(0x28, 1, 24.0f);
  g_bus.devs[0].first85 = true;
  AlashDS18B20 p(4);
  p.begin();
  CHECK(near(p.temperature(), 24.0f, 0.001f));

  // 11) постороннее устройство 1-Wire на линии не мешает и не считается датчиком
  fresh();
  g_bus.addDevice(0x01, 5, 0.0f);
  g_bus.addDevice(0x28, 1, 26.0f);
  AlashDS18B20 o(4);
  CHECK(o.begin());
  CHECK(o.count() == 1);
  CHECK(near(o.temperature(), 26.0f, 0.001f));

  // 12) ошибки: нет датчиков, замыкание, битый CRC
  fresh();
  AlashDS18B20 e(4);
  CHECK(!e.begin());
  CHECK(e.error() == AlashDS18B20::ERR_NO_SENSOR);
  CHECK(isnan(e.temperature()));
  g_bus.shorted = true;
  CHECK(!e.begin());
  CHECK(e.error() == AlashDS18B20::ERR_SHORT);
  fresh();
  g_bus.addDevice(0x28, 1, 20.0f);
  AlashDS18B20 bad(4);
  bad.begin();
  g_bus.devs[0].corrupt = true;
  CHECK(isnan(bad.temperature()));
  CHECK(bad.error() == AlashDS18B20::ERR_CRC);
  g_bus.devs[0].corrupt = false;
  g_now += 2000;
  CHECK(near(bad.temperature(), 20.0f, 0.001f));          // после починки линии снова работает

  // 13) больше датчиков, чем ALASH_DS18B20_MAX (в тесте 4): лишние пропускаются
  fresh();
  for (int i = 0; i < 6; i++) g_bus.addDevice(0x28, 20 + i, 10.0f + i);
  AlashDS18B20 many(4);
  CHECK(many.begin());
  CHECK(many.count() == 4);

  // 14) тексты ошибок на трёх языках не пустые и разные
  e.setLanguage(AlashDS18B20::LANG_RU); const char *ru = (const char *)e.errorText();
  e.setLanguage(AlashDS18B20::LANG_KK); const char *kk = (const char *)e.errorText();
  e.setLanguage(AlashDS18B20::LANG_EN); const char *en = (const char *)e.errorText();
  CHECK(strlen(ru) > 5 && strlen(kk) > 5 && strlen(en) > 5);
  CHECK(strcmp(ru, kk) != 0 && strcmp(kk, en) != 0);

  // 15) diagnose() работает и печатает адрес и температуру
  fresh();
  g_bus.addDevice(0x28, 1, 27.25f);
  AlashDS18B20 d(4);
  d.begin();
  printf("--- diagnose ---\n");
  d.diagnose(*(new Print));
  printf("--- end ---\n");

  if (fails) { printf("FAILED: %d\n", fails); return 1; }
  printf("ALL TESTS PASSED\n");
  return 0;
}
