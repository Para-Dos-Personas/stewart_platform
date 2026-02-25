import peasy.*; //<>//
import controlP5.*;
import processing.serial.*;
import hypermedia.net.*;

// =================================================================
// UDP + SERIAL GLOBALS
// =================================================================
UDP udp;
final int UDP_PORT = 5005;

Serial myPort;
boolean commStarted = false;

// Hand gesture data
float handPitch = 0;
float handRoll = 0;
final float MAX_ROTATION = PI/4;

// UI + platform
ControlP5 cp5;
PeasyCam camera;
Platform mPlatform;
Textlabel connectionLabel;

// Motion vars
float posX=0, posY=0, posZ=0, rotX=0, rotY=0, rotZ=0;
boolean ctlPressed = false;
long lastTime = 0;

// =================================================================
// SETUP  (ONLY GRAPHICS + UI HERE)
// =================================================================
void setup() {
  size(1024, 768, P3D);
  smooth();
  frameRate(60);
  textSize(20);

  // Camera
  camera = new PeasyCam(this, 666);
  camera.setRotations(-1.0f, 0.0f, 0.0f);
  camera.lookAt(8.0f, -50.0f, 80.0f);

  // Platform
  mPlatform = new Platform(1);
  mPlatform.applyTranslationAndRotation(new PVector(), new PVector());

  // UI
  cp5 = new ControlP5(this);

  cp5.addSlider("posX")
    .setPosition(20, 20)
    .setSize(180, 40).setRange(-1, 1);

  cp5.addSlider("posY")
    .setPosition(20, 70)
    .setSize(180, 40).setRange(-1, 1);

  cp5.addSlider("posZ")
    .setPosition(20, 120)
    .setSize(180, 40).setRange(-1, 1);

  cp5.addSlider("rotZ")
    .setPosition(width-200, 120)
    .setSize(180, 40).setRange(-1, 1);

  connectionLabel = cp5.addLabel("Hand Control: Inactive")
                       .setPosition(width-200, 20);

  cp5.setAutoDraw(false);
  camera.setActive(true);
}

// =================================================================
// DRAW
// =================================================================
void draw() {

  // ✅ Start UDP + Serial AFTER window is alive
  if (!commStarted) {
    initComms();
    commStarted = true;
  }

  background(200);

  // Apply hand rotations
  rotY = map(handPitch, -90, 90, -1, 1);
  rotX = map(handRoll, -90, 90, -1, 1);

  mPlatform.applyTranslationAndRotation(
    PVector.mult(new PVector(posX, posY, posZ), 50),
    PVector.mult(new PVector(rotX, rotY, rotZ), MAX_ROTATION)
  );

  mPlatform.draw();

  // UI overlay
  hint(DISABLE_DEPTH_TEST);
  camera.beginHUD();
  cp5.draw();
  camera.endHUD();
  hint(ENABLE_DEPTH_TEST);

  // Send servo data periodically
  if (millis() - lastTime > 100) {
    sendAngles();
    lastTime = millis();
  }
}

// =================================================================
// SAFE COMMUNICATION INIT (RUNS AFTER GL START)
// =================================================================
void initComms() {

  // UDP
  udp = new UDP(this, UDP_PORT);
  udp.listen(true);
  println("✅ UDP server started on port " + UDP_PORT);

  // Serial
  println("Available serial ports:");
  println(Serial.list());

  String portName = "COM4";  // change if needed

  try {
    myPort = new Serial(this, portName, 115200);
    println("✅ Serial port " + portName + " opened successfully.");

    delay(2500);  // now safe (window already running)

    myPort.write("A1:60;A2:60;A3:40;A4:60;A5:60;A6:40;\n");
    println(">>> sent test packet");

  } catch (Exception e) {
    println("❌ Error opening serial port " + portName);
    e.printStackTrace();
  }
}

// =================================================================
// UDP RECEIVE
// =================================================================
void receive(byte[] data, String ip, int port) {
  String message = new String(data);

  if (message.startsWith("A") && message.endsWith("*")) {
    message = message.substring(1, message.length() - 1);
    String[] values = split(message, ',');

    if (values.length == 2) {
      try {
        handPitch = float(values[0]);
        handRoll = float(values[1]);
        connectionLabel.setText("Hand Control: Active");
      } 
      catch (Exception e) {
        println("UDP parse error: " + message);
      }
    }
  }
}

// =================================================================
// SERIAL SEND
// =================================================================
void sendAngles() {
  if (myPort == null) return;

  float[] angles = mPlatform.getServoAngles();

  StringBuilder sb = new StringBuilder();
  for (int i = 0; i < angles.length; i++) {
    int angle = (int)angles[i];
    if (Float.isNaN(angle)) return;
    sb.append("A").append(i+1).append(":").append(angle).append(";");
  }

  myPort.write(sb.toString() + "\n");
  println("Sent: " + sb.toString());
}

// =================================================================
// UI + INPUT
// =================================================================
void controlEvent(ControlEvent theEvent) {
  camera.setActive(false);
}

void mouseReleased() {
  camera.setActive(true);
}

void keyPressed() {
  if (key == ' ') {
    camera.setRotations(-1.0f, 0.0f, 0.0f);
    camera.lookAt(8.0f, -50.0f, 80.0f);
    camera.setDistance(666);
  } 
  else if (keyCode == CONTROL) {
    camera.setActive(false);
    ctlPressed = true;
  }
}

void keyReleased() {
  if (keyCode == CONTROL) {
    camera.setActive(true);
    ctlPressed = false;
  }
}
