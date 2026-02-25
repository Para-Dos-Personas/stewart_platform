#include <DynamixelShield.h>
#include <SoftwareSerial.h>
SoftwareSerial esp_serial(4, 5);
const float DXL_PROTOCOL_VERSION = 1.0;
DynamixelShield dxl;
using namespace ControlTableItem;
String incomingData = "";

#define MIN_ANGLE 20
#define MAX_ANGLE 105

int angleToPulse(int angle) {
  return map(angle, 0, 180, 0, 1023);
}

void setup() {
  esp_serial.begin(9600);
  dxl.begin(1000000);
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);
  for (uint8_t id = 1; id <= 6; id++) {
    dxl.torqueOff(id);
    dxl.setOperatingMode(id, OP_POSITION);
    dxl.writeControlTableItem(P_GAIN, id, 16);
    dxl.writeControlTableItem(I_GAIN, id, 0);
    dxl.writeControlTableItem(D_GAIN, id, 0);
    // dxl.writeControlTableItem(MOVING_SPEED, id, 100); // add this
    dxl.torqueOn(id);
    dxl.setGoalPosition(id, angleToPulse(60));
  }
}

void loop() {
  while (esp_serial.available()) {
    char c = esp_serial.read();
    if (c == '\n') {
      parseAndMoveServos(incomingData);
      incomingData = "";
    } else {
      incomingData += c;
    }
  }
}

void parseAndMoveServos(String data) {
  int start = 0;
  while (true) {
    int end = data.indexOf(';', start);
    if (end == -1) break;
    String token = data.substring(start, end);
    if (token.startsWith("A")) {
      int colonIndex = token.indexOf(':');
      if (colonIndex != -1) {
        int motorID = token.substring(1, colonIndex).toInt();
        int angle = token.substring(colonIndex + 1).toInt();

        // Clamp before inversion
        angle = constrain(angle, MIN_ANGLE, MAX_ANGLE);

        if (motorID >= 1 && motorID <= 6) {
          int finalAngle = angle;
          if (motorID == 2 || motorID == 4 || motorID == 6) {
            finalAngle = 180 - angle;
            // Clamp again after inversion
            finalAngle = constrain(finalAngle, 180 - MAX_ANGLE, 180 - MIN_ANGLE);
          }
          dxl.setGoalPosition(motorID, angleToPulse(finalAngle));
        }
      }
    }
    start = end + 1;
  }
}