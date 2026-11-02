// [ru] AlashDS18B20 с паразитным питанием: датчик подключён двумя проводами (VDD и GND соединены вместе), питание берёт с линии данных.
//      Резистор 4,7 кОм между DATA и питанием обязателен. Библиотека сама узнаёт о таком питании в begin()
//      и на время измерения держит линию на питании. Если датчиков много или провода длинные, подайте питание на линию
//      через полевой транзистор: setPowerPin(пин, true).
// [kk] AlashDS18B20 паразиттік қуатпен: датчик екі сыммен қосылған (VDD мен GND бірге), қуатты дерек желісінен алады.
//      DATA мен қуат арасында 4,7 кОм резистор міндетті. Кітапхана begin() кезінде мұндай қуатты өзі біледі
//      және өлшеу кезінде желіні қуатта ұстайды. Датчик көп не сымдар ұзын болса, желіге қуатты өрістік транзистор
//      арқылы беріңіз: setPowerPin(пин, true).
// [en] AlashDS18B20 with parasite power: the sensor has two wires (VDD and GND tied together) and takes power from the data line.
//      A 4.7 kOhm resistor between DATA and the supply is required. The library detects this power mode in begin()
//      and holds the line high during each measurement. With many sensors or long wires, feed the line through
//      a MOSFET instead: setPowerPin(pin, true).
#include <AlashDS18B20.h>

const int PIN_DATA = 4;
AlashDS18B20 sensors(PIN_DATA);

void setup() {
  Serial.begin(115200);
  // sensors.setPowerPin(5);              // ключ питания на пине 5 (раскомментируйте, если он есть)
  // sensors.setPowerMode(AlashDS18B20::POWER_PARASITE);   // принудительно, если автоопределение ошиблось
  if (!sensors.begin()) {
    Serial.println(sensors.errorText());
    while (true) delay(1000);
  }
  Serial.print(F("Sensors: "));
  Serial.println(sensors.count());
  Serial.println(sensors.isParasite() ? F("Power: parasite (two wires)") : F("Power: external (three wires)"));
}

void loop() {
  float t = sensors.temperature();
  if (isnan(t)) Serial.println(sensors.errorText());
  else {
    Serial.print(t, 2);
    Serial.println(F(" C"));
  }
  delay(1000);
}
