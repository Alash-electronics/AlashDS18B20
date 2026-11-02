// [ru] AlashDS18B20: самый простой пример. Температура с датчика DS18B20 раз в секунду.
//
//      Подключение (датчик в водонепроницаемом корпусе, три провода):
//        красный  -> 3.3 В (Uno: 5 В)
//        чёрный   -> GND
//        жёлтый   -> пин 4 (Uno: D4, ESP32: GPIO4, ESP8266: D2)
//        резистор 4.7 кОм между жёлтым проводом и красным (подтяжка линии данных)
// [kk] AlashDS18B20: ең қарапайым мысал. DS18B20 датчигінен секунд сайын температура.
//
//      Қосу (су өткізбейтін корпустағы датчик, үш сым):
//        қызыл   -> 3.3 В (Uno: 5 В)
//        қара    -> GND
//        сары    -> 4 пин (Uno: D4, ESP32: GPIO4, ESP8266: D2)
//        сары және қызыл сымдар арасында 4.7 кОм резистор (деректер желісінің тартқышы)
// [en] AlashDS18B20: the simplest example. Temperature from a DS18B20 sensor once a second.
//
//      Wiring (a waterproof sensor, three wires):
//        red    -> 3.3 V (Uno: 5 V)
//        black  -> GND
//        yellow -> pin 4 (Uno: D4, ESP32: GPIO4, ESP8266: D2)
//        a 4.7 kOhm resistor between the yellow wire and the red one (data line pull-up)
#include <AlashDS18B20.h>

AlashDS18B20 sensors(4);   // пин, к которому подключён жёлтый провод

void setup() {
  Serial.begin(115200);
  if (!sensors.begin()) {                 // ищет датчик сам, адрес знать не нужно
    Serial.println(sensors.errorText());  // объяснит, что проверить
    while (true) delay(1000);
  }
}

void loop() {
  Serial.print("Температура: ");
  Serial.print(sensors.temperature());    // °C (NAN, если не получилось)
  Serial.println(" C");
  delay(1000);
}
