#include "BluetoothSerial.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

BluetoothSerial SerialBT;
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN 150  // 0 degrees
#define SERVOMAX 600  // 180 degrees

float pitch = 0.0, roll = 0.0, yaw = 0.0;
String input = "";
char currentMsgType = '\0';

int angleToPulse(int angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32-BT");
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(1000);

  for (int i = 0; i < 6; i++) {
    pwm.setPWM(i, 0, angleToPulse(90));
  }

  Serial.println("System Ready. Waiting for Bluetooth data...");
}

void loop() {
  while (SerialBT.available()) {
    char c = SerialBT.read();

    if (c == 'A' || c == 'D') {
      currentMsgType = c;
      input = "";
    } else if ((currentMsgType == 'A' && c == '*') || (currentMsgType == 'D' && c == '&')) {
      if (currentMsgType == 'A') parsePitchRoll(input);
      else if (currentMsgType == 'D') parseYaw(input);
      input = "";
      currentMsgType = '\0';
    } else {
      input += c;
    }
  }

  // Debug output
  Serial.print("Roll: "); Serial.print(roll);
  Serial.print(" | Pitch: "); Serial.print(pitch);
  Serial.print(" | Yaw: "); Serial.println(yaw);

  // Apply motion to servos (test logic)
  int rollAdj  = constrain((int)(roll / 2.0), -90, 90);
  int pitchAdj = constrain((int)(pitch / 2.0), -90, 90);
  int yawAdj   = constrain((int)(yaw / 2.0), -90, 90);

  for (int i = 0; i < 6; i++) {
    int angle = 90;

    // Simple demo logic to simulate effects
    if (i % 2 == 0) angle += rollAdj;
    else angle += pitchAdj;

    if (i == 0 || i == 5) angle += yawAdj;  // Slight yaw effect

    pwm.setPWM(i, 0, angleToPulse(constrain(angle, 0, 180)));
  }

  delay(100);
}

void parsePitchRoll(String data) {
  int commaIndex = data.indexOf(',');
  if (commaIndex != -1) {
    roll = data.substring(0, commaIndex).toFloat();
    pitch = data.substring(commaIndex + 1).toFloat();
  }
}

void parseYaw(String data) {
  yaw = data.toFloat();
}
