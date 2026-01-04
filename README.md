# ESP32 IoT Device – MQTTs & OTA Enabled


This repository contains an ESP32-based IoT device firmware built to behave like a real production IoT client, with a strong focus on reliability, security, and long-term maintainability.  
It connects securely to a backend using **MQTT over TLS**, supports **remote commands**, and allows **OTA firmware updates**.


The project is part of a larger system including a backend API and a simple UI, but the main focus here is the **ESP32 firmware design and communication flow**.

---

## Main Features

- Secure connection using **MQTTs (TLS)**
- Command-based control via MQTT topics
- **OTA firmware update** support
- Modular firmware structure
- Task-based design using FreeRTOS
- Sensor management with enable/disable control
- Commands are decoupled using a message queue to avoid blocking system tasks
- Focused on clarity and robustness rather than feature completeness

---
## High-Level Flow
The firmware is intentionally designed to stay responsive during unstable network conditions and OTA updates, using non-blocking mechanisms and task decoupling.

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

This project is **actively under development**.  
The current version represents a stable and working baseline. Ongoing work focuses on improving clarity, robustness, and long-term maintainability rather than adding new features.

---

## Related Components

- Backend service (MQTT + API)
- Simple UI for sending commands

---

## Purpose

This project is built as a **practical IoT system**, focusing on real-world communication, security, and maintainable firmware design rather than experimental features.
