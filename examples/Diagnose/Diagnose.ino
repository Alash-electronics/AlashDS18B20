// [ru] AlashDS18B20: датчик не работает? Этот скетч покажет, что видит плата на линии данных.
// [kk] AlashDS18B20: датчик жұмыс істемей ме? Бұл скетч тақта деректер желісінде не көріп тұрғанын көрсетеді.
// [en] AlashDS18B20: the sensor does not work? This sketch shows what the board sees on the data line.
#include <AlashDS18B20.h>

AlashDS18B20 sensors(4);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensors.setLanguage(AlashDS18B20::LANG_RU);   // LANG_KK: қазақша, LANG_EN: English
  sensors.begin();
  sensors.diagnose(Serial);                     // линия, найденные датчики, температура, подсказки
}

void loop() {}
