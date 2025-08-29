import peasy.*; //<>//
import controlP5.*;
import processing.serial.*;
import hypermedia.net.*; // Import the UDP library

// =================================================================
// 1. UDP SETUP
// =================================================================
UDP udp; // UDP object to receive data
final int UDP_PORT = 5005; // Must match the port in your Python script

// These will be updated by hand gestures
float handPitch = 0;
float handRoll = 0;

// Constants for mapping hand gestures
float MAX_ROTATION = PI/4; // Reduced max rotation for more stable control

// --- Original Global Variables ---
ControlP5 cp5;
PeasyCam camera;
Platform mPlatform;
Serial myPort;
Textlabel connectionLabel;

float posX=0, posY=0, posZ=0, rotX=0, rotY=0, rotZ=0;
boolean ctlPressed = false;
long lastTime = 0;

void setup() {
  size(1024, 768, P3D);
  smooth();
  frameRate(60);
  textSize(20);
  
  // =================================================================
  // 2. INITIALIZE UDP
  // =================================================================
  // Start listening for UDP packets on port 5005
  udp = new UDP(this, UDP_PORT);
  udp.listen(true);
  println("✅ UDP server started on port " + UDP_PORT);
  
  // --- Serial Port Setup ---
  println("Available serial ports:");
  println(Serial.list());
  String portName = "COM5"; // 🔁 IMPORTANT: Replace with your ESP32/Arduino port
  try {
    myPort = new Serial(this, portName, 115200);
    println("✅ Serial port " + portName + " opened successfully.");
    delay(2500); 
    myPort.write("A1:60;A2:60;A3:40;A4:60;A5:60;A6:40;\n");
    println(">>> sent test packet");
  } catch (Exception e) {
    println("❌ Error opening serial port " + portName + ". Check the port name and connection.");
    e.printStackTrace();
  }

  // --- Camera and Platform Setup ---
  camera = new PeasyCam(this, 666);
  camera.setRotations(-1.0f, 0.0f, 0.0f);
  camera.lookAt(8.0f, -50.0f, 80.0f);

  mPlatform = new Platform(1);
  mPlatform.applyTranslationAndRotation(new PVector(), new PVector());

  // --- ControlP5 UI Setup ---
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

void draw() {
  background(200);
  
  // =================================================================
  // 3. APPLY HAND GESTURE ROTATIONS
  // =================================================================
  rotY = map(handPitch, -90, 90, -1, 1);
  rotX = map(handRoll, -90, 90, -1, 1);
  
  mPlatform.applyTranslationAndRotation(
    PVector.mult(new PVector(posX, posY, posZ), 50),
    PVector.mult(new PVector(rotX, rotY, rotZ), MAX_ROTATION)
  );
  
  mPlatform.draw();

  // Draw UI
  hint(DISABLE_DEPTH_TEST);
  camera.beginHUD();
  cp5.draw();
  camera.endHUD();
  hint(ENABLE_DEPTH_TEST);
  
  // Send angles to Arduino/ESP32 periodically
  if (millis() - lastTime > 100) { // send data every 100ms
    sendAngles();
    lastTime = millis();
  }
}

// =================================================================
// 4. UDP EVENT HANDLER
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
        //println(String.format("Received -> Pitch: %.2f, Roll: %.2f", handPitch, handRoll));
        
      } catch (NumberFormatException e) {
        println("Could not parse UDP message: " + message);
      }
    }
  }
}


// =================================================================
// 5. SERIAL COMMUNICATION
// =================================================================
void sendAngles() {
  if (myPort == null) return;

  // Call the new method to get the remapped 0-180 degree servo angles.
  float[] angles = mPlatform.getServoAngles();

  StringBuilder sb = new StringBuilder();
  for (int i = 0; i < angles.length; i++) {
    // The 'angles' array now contains the final 0-180 value, so no more mapping is needed.
    // We just need to convert it to an integer.
    int angle = (int)angles[i];

    // Check for NaN just in case.
    if (Float.isNaN(angle)) return;

    sb.append("A").append(i + 1).append(":").append(angle).append(";");
  }

  myPort.write(sb.toString() + "\n");
  
  // This line will now print the sent data to the Processing console for debugging.
  println("Sent: " + sb.toString());
}


// --- UI and KEYBOARD EVENTS ---
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
  } else if (keyCode == CONTROL) {
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
