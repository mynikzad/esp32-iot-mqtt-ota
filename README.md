# ESP32 IoT Device – MQTTs & OTA Enabled

This project is an ESP32-based IoT firmware, designed to act like a real production client. The focus is on reliability, security, and keeping the code maintainable over time. 
It connects securely to a backend using **MQTT over TLS**, supports **remote commands**, and allows **OTA firmware updates**.


The project is part of a larger system including a backend API and a simple UI, but the main focus here is the **ESP32 firmware design and communication flow**.

---

## Main Features
The firmware isn’t overloaded with features — it keeps things clear and robust. Each part (WiFi, MQTT, OTA, sensors) is modular so you can follow the flow easily.

- Secure connection using **MQTTs (TLS)**
- Command-based control via MQTT topics
- **OTA firmware update** support
- Modular firmware structure
- Task-based design using FreeRTOS
- Sensor management with enable/disable control
- Commands are decoupled using a message queue to avoid blocking system tasks
- Focused on clarity and robustness rather than feature completeness

## Configuration Management

The firmware keeps device settings persistent across reboots using NVS.  
Each configuration block is validated with a CRC and version number, so if data gets corrupted or the structure changes, the system automatically falls back to safe defaults.  

This makes WiFi credentials, MQTT parameters, and other runtime options reliable and upgrade‑friendly.

---
## High-Level Flow
The boot-to-connection flow is kept simple: WiFi first, then secure MQTT, then tasks. This way the device stays responsive even if the network is shaky or an OTA update is running.

1. ESP32 boots and initializes system components  
2. Connects to WiFi  
3. Establishes a secure MQTT connection (TLS)  
4. Subscribes to command topics  
5. Processes incoming commands (control, configuration, OTA)  
6. Runs sensor and system tasks in an event-driven manner using non-blocking queues

---
The firmware is structured to keep networking, system logic, and hardware interaction clearly separated.


## Project Structure (ESP32)

esp32
main
   main
   wifi
   mqtt
   ota
   sensors
   system
   task
   msg_queue
   crash_counter
   command
   utils
   backoff
components
config
partitions




## Security Notes

- MQTT communication is encrypted using TLS
- Certificates are loaded on the device
- Sensitive values (WiFi, credentials) can be configured separately

Security is treated as a system-level concern and is continuously improved as the project evolves.
---

## Status
Right now the firmware is stable enough to run, but I’m still improving clarity and robustness step by step.
**actively under development**.
---

## Related Components

- Backend service (MQTT + API)
- Simple UI for sending commands

---

## Purpose

This project is built as a **practical IoT system**, focusing on real-world communication, security, and maintainable firmware design rather than experimental features.
