// AlashDS18B20: самый простой пример. Температура с датчика DS18B20 раз в секунду.
//
// Подключение (датчик в водонепроницаемом корпусе, три провода):
//   красный  -> 3.3 В (Uno: 5 В)
//   чёрный   -> GND
//   жёлтый   -> пин 4 (Uno: D4, ESP32: GPIO4, ESP8266: D2)
//   резистор 4.7 кОм между жёлтым проводом и красным (подтяжка линии данных)
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
