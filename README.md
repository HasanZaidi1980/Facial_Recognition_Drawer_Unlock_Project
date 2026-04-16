# Facial_Recognition_Drawer_Unlock_Project

### Biometric Secure Drawer System
An intelligent, two-board security system that utilizes Edge AI facial recognition to control a physical locking mechanism. This project uses an ESP32-S3 Sense as the vision controller and an ESP32 DevKit as the mechanical actuator, communicating wirelessly via the ESP-NOW protocol.

### Overview
The system remains in a low-power state until a user interacts with it. Upon a button trigger, the camera unit analyzes the user's face. If the face matches an enrolled "Face ID" stored in the local memory, a wireless command is sent to the receiver board to trigger a relay and retract a 12V solenoid lock.

### Hardware Components
Unit A: The Vision Transmitter (Camera Unit)
Controller: Seeed Studio XIAO ESP32-S3 Sense

### Peripherals: * OV3660 Camera Sensor
XIAO Expansion Board (Solderless)
Tactile Buttons (Enroll & Trigger)
Indicator LED (Status feedback)

### Unit B: The Mechanical Receiver (Lock Unit)
Controller: ESP32 DevKit V1
Peripherals:
5V Relay Module
12V Solenoid Lock
12V to 5V Buck Converter (for power regulation)
Flyback Diode (for circuit protection)

### AI Models & Software
This project leverages the ESP-WHO framework to perform facial inference entirely at the "Edge" (locally on the chip), ensuring maximum privacy and speed.

Face Detection: MTCNN (Multi-task Cascaded Convolutional Networks).
Face Recognition: MobileFaceNet (Optimized CNN for microcontrollers).
Communication: ESP-NOW (Low-latency, peer-to-peer Wi-Fi protocol).

### Code Files
ESP32_Sense_Camera.ino
Located in the transmitter unit, this code handles:

Enrollment Mode: Captures 5 samples of a user's face to create a 128-bit mathematical "Face ID" vector.
Recognition Mode: Captures a live frame, extracts feature vectors, and compares the "Euclidean distance" against the stored IDs.
Signal Transmission: If a match is found, it sends an encrypted "UNLOCK" string to the specific MAC address of the Receiver.

ESP32_DevKit_Receiver.ino
Located in the lock unit, this code handles:

Signal Monitoring: Constantly listens for ESP-NOW packets in the air.
Security Validation: Checks if the incoming message matches the secret "UNLOCK" command.
Mechanical Trigger: Activates the RELAY_PIN for 3 seconds to retract the solenoid bolt.

### Installation & Setup
MAC Address Identification:
Upload a simple MAC address scanner to your ESP32 DevKit.
Note the address and update the lockAddress[] array in the ESP32_Sense_Camera.ino file.

### Library Requirements:
Ensure the ESP32 board manager (v2.0.x or higher) is installed in the Arduino IDE.
Select OPI PSRAM in the Tools menu for the XIAO ESP32-S3.

### Deployment:
Upload File 1 to the XIAO ESP32-S3.
Upload File 2 to the ESP32 DevKit.

### Usage
Enrollment: Press and hold the Enroll Button (GPIO 12). Stand still while the LED blinks. Once it stops, your face is saved to the local flash memory.
Unlocking: Press the Trigger Button (GPIO 13). The camera will scan for 15 seconds.
Access: When the LED double-blinks, the drawer will click open for 3 seconds before re-locking automatically.

### Security & Privacy
Zero Cloud: No images or biometric data are ever sent to the internet.
Encryption: The peer-to-peer connection is hardcoded to specific hardware IDs, preventing signal hijacking.
Fail-Secure: The 12V solenoid is "Normally Closed," meaning the drawer remains locked even if the power is disconnected.

### Authors
Project Team: 
Elijah GhayaEzra 
Ezra Bakatubia 
Jesus Chavez-espino 
Liqa Hasan Syed Mohammed Zaidi

Course: ITAI 2277
