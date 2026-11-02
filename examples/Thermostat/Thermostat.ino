// [ru] AlashDS18B20: термостат. Реле включает нагреватель, пока температура ниже заданной.
//      Гистерезис 0.5 °C не даёт реле щёлкать туда-сюда около порога.
//        Реле (модуль на 3.3-5 В): сигнал на пин 7 (Uno), GPIO5 (ESP32 / ESP8266: см. RELAY_PIN)
// [kk] AlashDS18B20: термостат. Температура берілгеннен төмен болғанда реле қыздырғышты қосады.
//      0.5 °C гистерезис релені шек маңында шиқылдатпайды.
//        Реле (3.3-5 В модуль): сигнал 7 пинге (Uno), GPIO5 (ESP32 / ESP8266: RELAY_PIN қараңыз)
// [en] AlashDS18B20: a thermostat. The relay switches the heater on while the temperature is below the set value.
//      A 0.5 °C hysteresis keeps the relay from clicking back and forth around the threshold.
//        Relay (3.3-5 V module): signal to pin 7 (Uno), GPIO5 (ESP32 / ESP8266: see RELAY_PIN)
#include <AlashDS18B20.h>

const float TARGET = 25.0;       // нужная температура, °C
const float HYSTERESIS = 0.5;    // зона нечувствительности, °C
#if defined(ESP32) || defined(ESP8266)
const uint8_t RELAY_PIN = 5;
#else
const uint8_t RELAY_PIN = 7;
#endif

AlashDS18B20 sensors(4);
bool heating = false;

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  if (!sensors.begin()) {
    Serial.println(sensors.errorText());
    while (true) delay(1000);
  }
}

void loop() {
  float t = sensors.temperature();
  if (isnan(t)) {                         // датчик отвалился: безопасно выключаем нагреватель
    heating = false;
    Serial.println(sensors.errorText());
  } else if (t < TARGET - HYSTERESIS) {
    heating = true;
  } else if (t > TARGET + HYSTERESIS) {
    heating = false;
  }
  digitalWrite(RELAY_PIN, heating ? HIGH : LOW);

  Serial.print(t);
  Serial.println(heating ? " C  нагрев включён" : " C  нагрев выключен");
  delay(500);
}
