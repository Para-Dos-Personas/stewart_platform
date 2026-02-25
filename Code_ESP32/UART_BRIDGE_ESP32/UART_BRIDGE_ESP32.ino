const int LED_PIN = 2;   // Built-in LED on most ESP32 boards

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 25); // RX=16, TX=25
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    Serial2.write(c);

    // Blink LED on activity
    digitalWrite(LED_PIN, HIGH);
    delay(10);                 // short visible flash
    digitalWrite(LED_PIN, LOW);
  }
}