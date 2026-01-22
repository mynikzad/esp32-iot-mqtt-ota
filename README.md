## Design Decisions
## Known Limitations
## Future Work

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
- WiFi credentials are configured via menuconfig (not hardcoded)
- Modular firmware structure
- Task-based design using FreeRTOS
- Sensor management with enable/disable control
- Commands and sensor data are decoupled using a bounded message queue with back-pressure and overflow handling
- Focused on clarity and robustness rather than feature completeness

## Configuration Management

The firmware keeps device settings persistent across reboots using NVS.  
Each configuration block is validated with a CRC and version number, so if data gets corrupted or the structure changes, the system automatically falls back to safe defaults.  

This makes WiFi credentials, MQTT parameters, and other runtime options reliable and upgrade-friendly.
Runtime changes (for example via MQTT commands) are immediately persisted, so the device always restarts in a known, consistent state.

---

## State Layer Architecture (Persistent, Reproducible Behavior)

With this update, the firmware now follows a **state-driven architecture**.  
Instead of directly controlling hardware through incoming commands, all behavior is now derived from a central **State Manager** that acts as the single source of truth.

### Core Principles

- **Commands modify state — not hardware directly.**
- Every state change (like `sample_interval` or `sensor_enabled`) is written to NVS for persistence.
- On boot, the system calls `state_apply()` to restore these values to hardware.
- The current state is automatically published to MQTT retained topics.
- Hardware reacts only through state transitions, making reboot and reconnection perfectly consistent.

This guarantees:
- Full **reproducibility** after reboot  
- **Single point of control** for each hardware feature  
- Clear separation between *Command Handling*, *State Persistence*, and *Hardware Drivers*  

### Example

| Function type               | Responsibility                                              |
|------------------------------|-------------------------------------------------------------|
| `config_set_*()`             | Save value to NVS and validate with CRC                    |
| `state_update_*()`           | Update runtime state, apply side effects, publish MQTT     |
| `state_apply()`              | Restore state after boot and apply to hardware             |

This layer ensures that the ESP32 behaves deterministically across MQTT reconnects, OTA updates, and power cycles.

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
## Message Flow & Back-Pressure

Incoming commands and outgoing telemetry are passed through a bounded FreeRTOS queue.  
This prevents fast producers (for example sensors or MQTT callbacks) from overwhelming slower consumers.

When the queue is full:
- A short back-pressure delay is applied to the producer.
- Less important messages (such as state or telemetry) may be dropped.
- Control commands are prioritized to keep the device responsive.

This keeps the system stable under burst load and avoids memory growth or task starvation.


The firmware is structured to keep networking, system logic, and hardware interaction clearly separated.

## Commands update state.  
State is persisted and published.  
Hardware is driven only from state.

This keeps behavior consistent across reboots and MQTT reconnects.

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

- ### Build-time WiFi Configuration
   WiFi SSID and Password are now configurable via menuconfig (Kconfig.projbuild).
   This removes hardcoded credentials and improves maintainability and security.

Security is treated as a system-level concern and is continuously improved as the project evolves.
---

## Status
Right now the firmware is stable enough to run, but I’m still improving clarity and robustness step by step.  
Recent work focused on improving load behavior and resilience under high message rates.

The project is **actively under development**.
---

## Related Components

- Backend service (MQTT + API)
- Simple UI for sending commands

---

## Purpose

This project is built as a **practical IoT system**, focusing on real-world communication, security, and maintainable firmware design rather than experimental features.
