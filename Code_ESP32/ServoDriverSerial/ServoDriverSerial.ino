// Code on ESP32
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// =================================================================
// PCA9685 SETUP
// =================================================================
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN 150  // 0 degrees pulse
#define SERVOMAX 600  // 180 degrees pulse

// A string to store incoming data from the serial port
String incomingData = "";

// =================================================================
// HELPER: Convert angle to PCA9685 pulse width
// =================================================================
int angleToPulse(int angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

void setup() {
  Serial.begin(115200);

  // Init PCA9685
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(1000);

  // Set all 6 servos to neutral 90 degrees on startup
  for (int i = 0; i < 6; i++) {
    pwm.setPWM(i, 0, angleToPulse(90));
  }

  Serial.println("✅ ESP32 + PCA9685 Ready. Waiting for Processing data...");
}

void loop() {
  // Check if there is data available from the serial port
  while (Serial.available()) {
    char c = Serial.read();

    // If we receive a newline character, the message is complete
    if (c == '\n') {
      parseAndMoveServos(incomingData);
      incomingData = ""; // Clear the string for the next message
    } else {
      incomingData += c; // Add the character to our message string
    }
  }
}

// =================================================================
// PARSE DATA AND MOVE SERVOS
// Expects format: "A1:90;A2:85;A3:70;A4:90;A5:85;A6:70;"
// =================================================================
void parseAndMoveServos(String data) {
  Serial.print("Received: ");
  Serial.println(data);

  int start = 0;

  while (true) {
    int end = data.indexOf(';', start);
    if (end == -1) break; // No more commands

    String token = data.substring(start, end); // e.g., "A1:60"

    if (token.startsWith("A")) {
      int colonIndex = token.indexOf(':');
      if (colonIndex != -1) {
        // Extract servo index (0-based) and angle
        int servoIndex = token.substring(1, colonIndex).toInt() - 1; // "A1" -> 0
        int angle = token.substring(colonIndex + 1).toInt();

        // Safety clamp
        angle = constrain(angle, 0, 180);

        // Valid servo index check (PCA9685 channels 0-5)
        if (servoIndex >= 0 && servoIndex < 6) {
          pwm.setPWM(servoIndex, 0, angleToPulse(angle));
          Serial.printf("Servo %d set to %d degrees\n", servoIndex + 1, angle);
        }
      }
    }

    start = end + 1; // Move to next command
  }
}