#include <Arduino.h>

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.printf("Hello, world!\n");
  delay(1000);
}
