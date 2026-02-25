void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);  // match the Uno's SoftwareSerial baud
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    Serial2.write(c);
  }
}