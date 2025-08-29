// This code is adapted for an Arduino Mega with a standard sensor shield.
// It INCLUDES the angle correction for physically mirrored servos.

#include <Servo.h>  // Using the standard Servo library for Arduino

// =================================================================
// 1. SERVO PIN ASSIGNMENTS
// =================================================================
// These are common, PWM-capable pins easily accessible on a sensor shield.
const int servoPins[6] = { 8, 9, 10, 11, 12, 13 };
const int LED_PIN = 13;  // built-in LED


// An array of Servo objects
Servo servos[6];

// A string to store incoming data from the serial port
String incomingData = "";

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // =================================================================
  // 2. ATTACH SERVOS TO PINS
  // =================================================================
  for (int i = 0; i < 6; i++) {
    servos[i].attach(servoPins[i]);
    if (i == 0 || i == 2 || i == 4) {
      servos[i].write(120);
    } else {
      servos[i].write(60);  // Set all servos to a neutral 90-degree position
    }
  }

  Serial.println("✅ Arduino Mega Ready. Servos attached.");
  Serial.println("Waiting for Processing data...");
}

void loop() {
  // Check if there is data available from the serial port
  while (Serial.available()) {
    char c = Serial.read();

    // If we receive a newline character, the message is complete
    if (c == '\n') {
      parseAndMoveServos(incomingData);
      incomingData = "";  // Clear the string for the next message
    } else {
      incomingData += c;  // Add the character to our message string
    }
  }
}

// =================================================================
// 3. PARSE DATA AND MOVE SERVOS
// =================================================================
void parseAndMoveServos(String data) {
  Serial.print("Received: ");
  digitalWrite(LED_PIN, HIGH);
  delay(30);
  digitalWrite(LED_PIN, LOW);
  Serial.println(data);
  int servoIndex;
  int angle;

  int start = 0;
  while (true) {
    int end = data.indexOf(';', start);
    if (end == -1) break;  // Exit the loop if no more commands are found

    String token = data.substring(start, end);  // e.g., "A1:60"

    if (token.startsWith("A")) {
      int colonIndex = token.indexOf(':');
      if (colonIndex != -1) {
        servoIndex = token.substring(1, colonIndex).toInt() - 1;  // "A1" -> 0
        angle = token.substring(colonIndex + 1).toInt();

        // --- RE-ADDED ANGLE CORRECTION ---
        // This is necessary if servos 2, 4, and 6 are physically mounted
        // in a mirrored orientation compared to the others.
        if (servoIndex == 0 || servoIndex == 2 || servoIndex == 4) {
          angle = 180 - angle;
        }

        // Ensure the final angle is within the valid 0-180 range
        angle = constrain(angle, 0, 120);

        // Check for a valid servo index before moving
        if (servoIndex >= 0 && servoIndex < 6) {
          servos[servoIndex].write(angle);

          Serial.print("Servo ");
          Serial.print(servoIndex + 1);
          Serial.print(" set to ");
          Serial.print(angle);
          Serial.println(" degrees");
        }
      }
    }
    // Move to the start of the next command
    start = end + 1;
  }
}