// [ru] AlashDS18B20: измерение без остановки программы.
//      Обычный temperature() ждёт до 0.75 с, пока датчик измеряет. Если параллельно надо
//      мигать светодиодом или опрашивать кнопку, запустите измерение через request(),
//      занимайтесь другим и заберите результат, когда ready() станет true.
// [kk] AlashDS18B20: бағдарламаны тоқтатпай өлшеу.
//      Әдеттегі temperature() датчик өлшегенше 0.75 с-қа дейін күтеді. Егер қатар
//      жарықдиодты жыпылықтату немесе батырманы сұрау керек болса, өлшеуді request() арқылы іске қосыңыз,
//      басқа іспен айналысыңыз да, ready() true болғанда нәтижені алыңыз.
// [en] AlashDS18B20: measuring without stopping the program.
//      A normal temperature() waits up to 0.75 s while the sensor measures. If you also need to
//      blink an LED or poll a button, start the measurement with request(),
//      do other work and pick up the result when ready() becomes true.
#include <AlashDS18B20.h>

AlashDS18B20 sensors(4);
#ifndef LED_BUILTIN
#define LED_BUILTIN 2   // на некоторых платах ESP32 встроенный светодиод на GPIO2
#endif
const uint8_t LED = LED_BUILTIN;
uint32_t lastBlink = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  if (!sensors.begin()) {
    Serial.println(sensors.errorText());
    while (true) delay(1000);
  }
  sensors.request();                      // первое измерение
}

void loop() {
  if (sensors.ready()) {                  // измерение закончено
    Serial.print("Температура: ");
    Serial.println(sensors.temperature()); // читается сразу, без ожидания
    sensors.request();                    // запускаем следующее
  }

  // Параллельно светодиод мигает 5 раз в секунду: программа не стоит на месте
  if (millis() - lastBlink >= 100) {
    lastBlink = millis();
    digitalWrite(LED, !digitalRead(LED));
  }
}
