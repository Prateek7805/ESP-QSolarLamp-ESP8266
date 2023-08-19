#include "QWiFi_IoT.h"



//server certificate
X509List cert(IRG_Root_X1);

lightData led;
QWiFi_IoT qw(80, &led);
void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();
  pinMode(2, OUTPUT);

  qw.begin();
}

void loop() {
  qw.handleStatus(&cert, 500);
  analogWrite(2, led.power? map(led.brightness, 0, 100, 255, 0) : 255);
}
