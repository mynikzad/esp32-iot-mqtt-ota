# Architecture Notes

This project is built as a simple but practical IoT setup.

- The ESP32 is responsible for running the device logic, handling sensors, and reacting to incoming commands.
- Communication between the device and the server is done using MQTT over TLS to keep the connection secure.
- Firmware updates are handled through OTA, so the device can be updated remotely without physical access.
- The backend manages MQTT messages and exposes APIs that are used by a basic UI to control the device.

The structure is intentionally kept simple to make the firmware easy to reason about, debug, and extend in real-world IoT deployments.
