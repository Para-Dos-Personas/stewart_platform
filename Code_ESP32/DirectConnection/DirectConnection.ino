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
// =================================================================
// 3. PARSE DATA AND MOVE SERVOS (CORRECTED)
// =================================================================
void parseAndMoveServos(String data) {
  Serial.print("Received: ");
  Serial.println(data);

  int servoIndex;
  int angle;

  int start = 0;
  while (true) {
    int end = data.indexOf(';', start);
    if (end == -1) break; // Exit the loop if no more commands are found

    String token = data.substring(start, end); // e.g., "A1:60"
    
    if (token.startsWith("A")) {
      int colonIndex = token.indexOf(':');
      if (colonIndex != -1) {
        // Extract the servo number and angle from the token
        servoIndex = token.substring(1, colonIndex).toInt() - 1; // "A1" -> 0
        angle = token.substring(colonIndex + 1).toInt();

        // ----------------------------------------------------
        // DELETED: The "if (servoIndex == 1 ...)" block that
        // inverted the angles has been REMOVED.
        // The Platform class already calculated the correct angle.
        // ----------------------------------------------------
        
        // We still constrain the angle as a safety measure
        angle = constrain(angle, 0, 180);

        // Check for a valid servo index before moving
        if (servoIndex >= 0 && servoIndex < 6) {
          servos[servoIndex].write(angle);
          Serial.printf("Servo %d set to %d degrees\n", servoIndex + 1, angle);
        } 
      }
    }
    // Move to the start of the next command
    start = end + 1;
  }
}