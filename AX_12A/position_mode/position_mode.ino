#include <DynamixelShield.h>
#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_MEGA2560)
  #include <SoftwareSerial.h>
  SoftwareSerial soft_serial(7, 8);
  #define DEBUG_SERIAL soft_serial
#elif defined(ARDUINO_SAM_DUE) || defined(ARDUINO_SAM_ZERO)
  #define DEBUG_SERIAL SerialUSB
#else
  #define DEBUG_SERIAL Serial
#endif

const float DXL_PROTOCOL_VERSION = 1.0;
DynamixelShield dxl;
using namespace ControlTableItem;

void setup() {
  DEBUG_SERIAL.begin(115200);
  dxl.begin(1000000);
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);

  for (uint8_t id = 1; id <= 6; id++) {
    dxl.ping(id);
    dxl.torqueOff(id);
    dxl.setOperatingMode(id, OP_POSITION);
    dxl.torqueOn(id);
  }
}

void loop() {
  for (uint8_t id = 1; id <= 6; id++) {
    DEBUG_SERIAL.print("Sweeping motor ID: ");
    DEBUG_SERIAL.println(id);

    // 0 degrees
    dxl.setGoalPosition(id, 0, UNIT_DEGREE);
    delay(1500);

    // 180 degrees
    dxl.setGoalPosition(id, 180, UNIT_DEGREE);
    delay(1500);

    // back to 90
    dxl.setGoalPosition(id, 90, UNIT_DEGREE);
    delay(1500);
  }
}