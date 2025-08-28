# Stewart Platform Project

This project implements a 6-DOF Stewart Platform, including simulation, hardware control for ESP32, and 3D printable models.

## Components

### 1. Simulation (`simulation/`)
- **[Platform.pde](simulation/Platform.pde)**: Implements the Stewart Platform kinematics and visualization in Processing.
- **[StewartSimulator.pde](simulation/StewartSimulator.pde)**: Main Processing sketch. Provides UI sliders, 3D visualization, UDP/Serial communication, and sends servo angles to the ESP32.
- **[sketch.properties](simulation/sketch.properties)**: Processing project configuration.

### 2. ESP32 Firmware (`Code_ESP32/`)
- **[DirectConnection.ino](Code_ESP32/DirectConnection.ino)**: Receives servo angle commands over serial and directly controls servos using the ESP32Servo library.
- **[ServoDriver.ino](Code_ESP32/ServoDriver.ino)**: Receives commands via Bluetooth Serial, parses pitch/roll/yaw, and drives servos using an Adafruit PWM driver.

### 3. 3D Models (`STL/`)
- **Legs_Model.stl**, **Straight_Connect.stl**: 3D printable parts for the Stewart Platform.
- **.print** and **.makerbot** files: Slicing and printer-specific files.

### 4. Python Utilities (`venv/`)
- **[main.py](venv/main.py)**: Bridges BLE (Bluetooth Low Energy) data from a phone to the ESP32 via serial.
- **[test.py](venv/test.py)**: Sends test servo commands to the ESP32 over serial for debugging.

## How It Works

1. **Simulation**:  
   Use Processing to simulate and visualize the Stewart Platform. Adjust translation and rotation via UI or UDP (e.g., from hand gesture data). The simulator computes servo angles and sends them to the ESP32.

2. **ESP32 Control**:  
   The ESP32 receives servo angles (either via serial or Bluetooth), parses them, and sets the servo positions accordingly.

3. **3D Printing**:  
   STL files are provided for printing the platform's mechanical parts.

4. **Python Scripts**:  
   - `main.py`: For relaying BLE data to the ESP32.
   - `test.py`: For sending test commands to the ESP32.

## Usage

### Simulation
- Open `simulation/StewartSimulator.pde` in Processing.
- Adjust the platform using UI sliders or send UDP hand gesture data.
- Connect the ESP32 via USB and set the correct serial port in the code.

### ESP32
- Flash either `DirectConnection.ino` or `ServoDriver.ino` to your ESP32, depending on your hardware setup.
- For Bluetooth control, pair your device and use the correct Bluetooth name.

### 3D Printing
- Print the STL files in the `STL/` directory using your preferred slicer and 3D printer.

### Python Scripts
- Use `main.py` to bridge BLE data to the ESP32.
- Use `test.py` to send test servo commands for debugging.

## Notes

- Update serial port names and BLE UUIDs as needed for your setup.
- The Processing simulation expects the ESP32 to be connected and ready to receive servo angle commands.

---

**Authors:**  
Varsha
Allen

