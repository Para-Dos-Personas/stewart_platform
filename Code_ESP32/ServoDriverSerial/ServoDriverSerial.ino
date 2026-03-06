#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// =================================================================
// PCA9685 SETUP
// =================================================================
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
#define SERVOMIN 150
#define SERVOMAX 600
#define ANGLE_MIN 25
#define ANGLE_MAX 120

// =================================================================
// SMOOTHING CONFIG — tune these to adjust feel
// =================================================================
#define MAX_STEP     2.0f   // Max degrees per update cycle (lower = slower/smoother)
#define SMOOTH_ALPHA 0.15f  // Low-pass filter weight (0.0=never moves, 1.0=no filter)
#define UPDATE_MS    2     // How often servos update in milliseconds

float currentAngle[6];  // Smoothed current angles
float targetAngle[6];   // Target angles received from Processing

String incomingData = "";

// =================================================================
// HELPER: Convert angle to PCA9685 pulse width
// =================================================================
int angleToPulse(int angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

// =================================================================
// HELPER: Apply inversion for servos 2, 4, 6
// =================================================================
int applyInversion(int servoIndex, int angle) {
  if (servoIndex == 1 || servoIndex == 3 || servoIndex == 5) {
    return 180 - angle;
  }
  return angle;
}

void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(1000);

  // Initialize all servos to 60 degrees
  for (int i = 0; i < 6; i++) {
    float angle = constrain(60, ANGLE_MIN, ANGLE_MAX);
    currentAngle[i] = angle;
    targetAngle[i]  = angle;
    pwm.setPWM(i, 0, angleToPulse(applyInversion(i, (int)angle)));
  }

  Serial.println("✅ ESP32 + PCA9685 Ready. Waiting for Processing data...");
}

void loop() {
  // --- Read incoming serial data ---
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      parseTargetAngles(incomingData);
      incomingData = "";
    } else {
      incomingData += c;
    }
  }

  // --- Smoothing update loop (runs every UPDATE_MS) ---
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= UPDATE_MS) {
    lastUpdate = millis();
    updateServos();
  }
}

// =================================================================
// SMOOTHING: Step current angles toward target and write to servos
// =================================================================
void updateServos() {
  for (int i = 0; i < 6; i++) {
    float diff = targetAngle[i] - currentAngle[i];

    // Slew rate limit — cap how much we move per cycle
    if (diff > MAX_STEP)       diff = MAX_STEP;
    else if (diff < -MAX_STEP) diff = -MAX_STEP;

    // Low-pass filter blended on top of the slew
    currentAngle[i] += SMOOTH_ALPHA * diff;

    // Write to servo
    int finalAngle = constrain((int)currentAngle[i], ANGLE_MIN, ANGLE_MAX);
    pwm.setPWM(i, 0, angleToPulse(applyInversion(i, finalAngle)));
  }
}

// =================================================================
// PARSE incoming data and update TARGET angles only
// Expects format: "A1:90;A2:85;A3:70;A4:90;A5:85;A6:70;"
// =================================================================
void parseTargetAngles(String data) {
  Serial.print("Received: ");
  Serial.println(data);

  int start = 0;
  while (true) {
    int end = data.indexOf(';', start);
    if (end == -1) break;

    String token = data.substring(start, end);
    if (token.startsWith("A")) {
      int colonIndex = token.indexOf(':');
      if (colonIndex != -1) {
        int servoIndex = token.substring(1, colonIndex).toInt() - 1;
        int angle = token.substring(colonIndex + 1).toInt();

        angle = constrain(angle, ANGLE_MIN, ANGLE_MAX);

        if (servoIndex >= 0 && servoIndex < 6) {
          targetAngle[servoIndex] = (float)angle;
          Serial.printf("Target Servo %d -> %d degrees\n", servoIndex + 1, angle);
        }
      }
    }
    start = end + 1;
  }
}