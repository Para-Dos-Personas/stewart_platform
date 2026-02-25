#include <SoftwareSerial.h>

const int LED_PIN = 13;          // UNO built-in LED
SoftwareSerial SerialOut(6, 7);  // RX, TX  → to second UNO

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);     // USB from PC
  SerialOut.begin(9600);    // to second UNO (match its baud)
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    SerialOut.write(c);
    delay(2); // small gap between bytes
    digitalWrite(LED_PIN, HIGH);
    delay(10);
    digitalWrite(LED_PIN, LOW);
  }
}