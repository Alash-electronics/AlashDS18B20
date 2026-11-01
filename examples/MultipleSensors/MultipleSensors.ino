// AlashDS18B20: несколько датчиков на одном проводе (одна подтяжка 4.7 кОм на всех).
// Библиотека находит все датчики сама. Номера идут в порядке адресов и не меняются,
// пока вы не добавили или не убрали датчик. Адрес из printAddress() можно записать
// в скетч, чтобы всегда знать, какой датчик где стоит.
#include <AlashDS18B20.h>

AlashDS18B20 sensors(4);

void setup() {
  Serial.begin(115200);
  if (!sensors.begin()) {
    Serial.println(sensors.errorText());
    while (true) delay(1000);
  }
  Serial.print("Найдено датчиков: ");
  Serial.println(sensors.count());
  for (uint8_t i = 0; i < sensors.count(); i++) {
    Serial.print("  #");
    Serial.print(i);
    Serial.print("  адрес ");
    sensors.printAddress(Serial, i);
    Serial.println();
  }
}

void loop() {
  // Все датчики измеряют одновременно: ждать надо один раз, а не по 0.75 с на каждый.
  for (uint8_t i = 0; i < sensors.count(); i++) {
    Serial.print("#");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(sensors.temperature(i));
    Serial.print("  ");
  }
  Serial.println();
  delay(1000);
}
