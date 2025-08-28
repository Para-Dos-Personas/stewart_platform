// MODIFIED: Using the ESP32Servo library for direct pin control
#include <ESP32Servo.h>

// =================================================================
// 1. SERVO PIN ASSIGNMENTS
// =================================================================
// An array to hold the GPIO pin numbers for each servo.
const int servoPins[6] = {13, 12, 14, 27, 26, 25};

// An array of Servo objects
Servo servos[6];

// A string to store incoming data from the serial port
String incomingData = "";

void setup() {
  Serial.begin(115200);

  // =================================================================
  // 2. ATTACH SERVOS TO PINS
  // =================================================================
  // Attach each servo in the array to its corresponding pin
  for (int i = 0; i < 6; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(90); // Set all servos to a neutral 90-degree position
  }

  Serial.println("✅ ESP32 Ready. Servos attached directly to GPIO pins.");
  Serial.println("Waiting for Processing data...");
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
// 3. PARSE DATA AND MOVE SERVOS
// =================================================================
// This function parses the incoming string (e.g., "A1:90;A2:85;...")
void parseAndMoveServos(String data) {
  Serial.print("Received: ");
  Serial.println(data);

  int servoIndex;
  int angle;

  int start = 0;
  while (true) {
    // Find the end of the current command (marked by ';')
    int end = data.indexOf(';', start);
    if (end == -1) break; // Exit the loop if no more commands are found

    String token = data.substring(start, end); // e.g., "A1:90"
    
    if (token.startsWith("A")) {
      int colonIndex = token.indexOf(':');
      if (colonIndex != -1) {
        // Extract the servo number and angle from the token
        servoIndex = token.substring(1, colonIndex).toInt() - 1; // Convert "A1" to index 0
        angle = token.substring(colonIndex + 1).toInt();

        // Apply correction for servos 2, 4, and 6 (which are at indices 1, 3, 5)
        // This is needed if they are mounted mirrored to the others.
        if (servoIndex == 1 || servoIndex == 3 || servoIndex == 5) {
          angle = 180 - angle;
        }
        
        // Ensure the final angle is within the valid 0-180 range
        angle = constrain(angle, 0, 180);

        // Check if the servo index is valid before moving the servo
        if (servoIndex >= 0 && servoIndex < 6) {
          // MODIFIED: Use the servo.write() command instead of pwm.setPWM()
          servos[servoIndex].write(angle);
          Serial.printf("Servo %d (Pin %d) set to %d degrees\n", servoIndex + 1, servoPins[servoIndex], angle);
        } 
      }
    }
    // Move to the start of the next command
    start = end + 1;
  }
}
