# ESP32 IoT Device – MQTTs & OTA Enabled

This repository contains an ESP32-based IoT device firmware that connects securely to a backend using **MQTT over TLS**, supports **remote commands**, and allows **OTA firmware updates**.

The project is part of a larger system including a backend API and a simple UI, but the main focus here is the **ESP32 firmware design and communication flow**.

---

## Main Features

- Secure connection using **MQTTs (TLS)**
- Command-based control via MQTT topics
- **OTA firmware update** support
- Modular firmware structure
- Task-based design using FreeRTOS
- Sensor management with enable/disable control

---

## High-Level Flow

1. ESP32 boots and initializes system components  
2. Connects to WiFi  
3. Establishes a secure MQTT connection (TLS)  
4. Subscribes to command topics  
5. Processes incoming commands (control, configuration, OTA)  
6. Runs sensor and system tasks in an event-driven manner  

---

## Project Structure (ESP32)

esp32/
├── main/
│ ├── wifi/
│ ├── mqtt/
│ ├── ota/
│ ├── sensors/
│ └── system/
├── components/
└── config/

---

## Security Notes

- MQTT communication is encrypted using TLS
- Certificates are loaded on the device
- Sensitive values (WiFi, credentials) can be configured separately

---

## Status

This project is **actively under development**.  
The current version represents a working and stable baseline, with ongoing improvements focused on structure, documentation, and robustness.

---

## Related Components

- Backend service (MQTT + API)
- Simple UI for sending commands

---

## Purpose

This project is built as a **practical IoT system**, focusing on real-world communication, security, and maintainable firmware design rather than experimental features.
