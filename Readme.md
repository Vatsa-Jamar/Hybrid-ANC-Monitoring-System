# Hybrid-ANC-Monitoring-System/(Smart Noise Cancellation & Secure IoT Monitoring System)

## Overview
This repository contains the source code and documentation for an advanced acoustic noise management system. The project explores and implements two parallel methodologies for tackling environmental noise pollution:
1. **Hardware Implementation (Analog ANC):** A low-latency, feedforward active noise cancellation circuit built using operational amplifiers (741) and BJT pre-amplifiers to achieve real-time destructive interference.
2. **Software Implementation (Digital IoT):** A secure data acquisition and monitoring pipeline using an ESP32 microcontroller. The system captures ambient noise, encrypts the data using AES-128-CBC, and uploads it to a ThingSpeak cloud dashboard. A MATLAB script fetches, decrypts, and processes the raw data into normalized decibel (dB) readings for analytical visualization.

*Course Project: Analog Circuits [BECE206L]*

## Key Features
* **Analog Signal Processing:** Custom-designed summing amplifiers and phase inverters for theoretical real-time noise cancellation.
* **End-to-End Encryption:** Implements 128-bit AES-CBC encryption directly on the ESP32 before Wi-Fi transmission, ensuring high data security.
* **Cloud Analytics:** Integration with ThingSpeak for data logging and a custom MATLAB script for outlier removal, data smoothing, and dB conversion.
* **Dynamic Geotagging:** The ESP32 utilizes IP-based geolocation to automatically update the ThingSpeak channel's physical coordinates.

## Hardware Architecture
* **Core Controller:** ESP-WROOM-32
* **Sensors:** Electret Condenser Microphone / KY-037 Sound Sensor
* **Analog ANC Circuit:** BC547BP BJT, 741 Op-Amps, assorted passive components.
* **Power Supply:** 18650 Li-ion Battery with TP4056 Charging Module (Digital), 9V Heavy-Duty Battery (Analog).

## Repository Structure
```text
├── firmware/
│   └── esp32_secure_monitor.ino        # ESP32 data acquisition and AES encryption code
├── software/
│   └── thingspeak_analysis_dashboard.m # MATLAB decryption and data processing script
├── hardware/
│   ├── analog_anc_schematic.png        # Multisim circuit design
│   └── connections.txt                 # Hardware wiring and pinouts
└── docs/
    └── Analog_Circuits_Project.pdf     # Full course project report
