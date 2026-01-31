# Executive Summary
This repository implements a **production-grade embedded IoT firmware** based on ESP32 and ESP-IDF.
The system is intentionally designed for **long-running, unattended field deployment**, where failures, reboots, network loss, and OTA updates are expected rather than exceptional.

Instead of demo-style feature implementation, the firmware emphasizes **deterministic behavior**, **explicit state management**, **safe recovery after failure**, and **production-oriented communication patterns**.


## Project Scope and Architecture
- **Platform:** ESP32 with ESP-IDF (v5.x), designed as a state-driven embedded system
- **Main functionalities:** 
The feature set is intentionally minimal and carefully selected to support reliability, debuggability, and predictable behavior in production environments.
WiFi connectivity, MQTT communication over TLS, sensor management (temperature, humidity, pressure), actuator PWM control, OTA update support, crash counters for safe mode.

- **Key modules:** `app_main.c` initializes WiFi, MQTT, sensors, manages tasks; `mqtt.c` handles secure MQTT; `config_manager.c` manages configurations stored in NVS; `crash_counter.c` tracks crashes within a time window.
- **Certificates:** Embedded in the binary (`cert.pem`, `certs_ca.crt`) for HTTPS/MQTT TLS.
- **Architecture Diagram:**
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

## State Layer Architecture (Single Source of Truth)

The firmware follows a **state-driven architecture**.  
Instead of directly controlling hardware through incoming commands, all behavior is derived from a central **State Manager** that acts as the single source of truth.

This approach is intentionally designed for long-running field deployment, where devices may reboot, reconnect, or recover from partial failure.


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
## Command & Acknowledgment Protocol

The device uses a structured JSON-based command/acknowledgment protocol over MQTT.

### Example Command
```json
{
  "req_id": "a1b2c3",
  "type": "command",
  "name": "set_led",
  "value": 1
}
```

### Example Acknowledgment
```json
{
  "req_id": "a1b2c3",
  "status": "ok",
  "code": 0,
  "ts": 1700000123,
  "cmd": "led",
  "data": {
    "state": 1
  }
}
```

Each acknowledgment includes:
- `status`: success or error
- `code`: stable numeric result code
- `ts`: device timestamp (ms)
- Optional error reason for failed commands

This protocol is designed for reliable field operation and backend integration.

---

## OTA Update & Rollback Strategy

Firmware updates are performed via a dedicated FreeRTOS task, decoupled from MQTT callbacks to avoid blocking system communication.

- OTA is delivered over HTTPS/MQTTs.
- Firmware integrity is validated using SHA-256.
- The ESP-IDF partition table enables automatic rollback on failed updates.
- If an update fails or the device crashes repeatedly after an update, the system safely reverts to the previous firmware version.

This design prevents device bricking and ensures recoverability in real-world deployments.

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

## Key Files and Their Responsibilities
- `app_main.c`: System initialization, WiFi, MQTT, sensors, task creation, crash check, OTA.
- `crash_counter.c`: Tracks and manages crash counts to enable Safe Mode.
- `config_manager.c`: Loads, saves, and validates configuration in NVS with CRC.
- `mqtt.c`: Manages MQTT connection, security, and command reception.
- `command.c`: Parses JSON commands, manages system actions (reboot, OTA, updates).
- `backoff.c`: Implements exponential backoff for retries.
- `actuator_pwm.c`: Currently empty; planned for PWM-based actuator control.


## Security and Certificates

- MQTT communication is encrypted using TLS
- Certificates are loaded on the device
- Sensitive values (WiFi, credentials) can be configured separately

Certificates (`cert.pem`, `certs_ca.crt`) are embedded into the binary via `CMakeLists.txt` to facilitate TLS-secured MQTT communication.  
For enhanced security, future plans include external storage or hardware security modules to protect private keys.

- ### Build-time WiFi Configuration
   WiFi SSID and Password are now configurable via menuconfig (Kconfig.projbuild).
   This removes hardcoded credentials and improves maintainability and security.
   

Security is treated as a system-level concern and is continuously improved as the project evolves.

## Status

The firmware is stable and actively evolving. Current development focuses on robustness under failure scenarios, deterministic behavior, and production-grade communication patterns.

---
### Current Status — Strengths
- Modular code structure separating WiFi, MQTT, sensors
- Secure MQTT connection with embedded TLS certificates
- Crash counter mechanism for fault detection
- OTA update support with version control
- Use of FreeRTOS tasks for multitasking


## Related Components

- Backend service (MQTT + API)
- Simple UI for sending commands

---

## Purpose

This project is built as a **practical IoT system**, focusing on real-world communication, security, and maintainable firmware design rather than experimental features.

The repository intentionally focuses on firmware-side architecture and reliability patterns rather than end-user features.



The project is **actively under development**.
---

## Build & Run Instructions
1. Install ESP-IDF v5.x and set up environment.
2. Clone the repository.
3. Configure using `idf.py menuconfig` if needed.
4. Build: `idf.py build`
5. Flash: `idf.py -p /dev/ttyUSB0 flash`
6. Monitor: `idf.py -p /dev/ttyUSB0 monitor`
This repository focuses on firmware design; backend and UI components are intentionally minimal.
