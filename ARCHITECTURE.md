# Embedded System Architecture

## Overview

This project implements a **production-grade ESP32 firmware** designed for long-term field deployment in IoT environments.
The system is architected to prioritize **deterministic behavior**, **fault tolerance**, and **safe remote lifecycle management** rather than feature density or rapid prototyping.

The firmware is built using ESP-IDF and FreeRTOS and reflects architectural decisions made under typical real-world constraints:
limited resources, unreliable connectivity, remote updates, and the necessity to recover autonomously from failures.

---

## System Goals

The primary architectural goals of this system are:

- Deterministic behavior across reboots, crashes, OTA updates, and network interruptions
- Safe and predictable interaction between cloud commands and physical hardware
- Field survivability: the device must recover without manual intervention
- Clear separation between communication, state management, and hardware control
- Production readiness over prototype simplicity

---

## Key Constraints

- MCU-class device (ESP32)
- Limited RAM and flash memory
- Intermittent or unstable network connectivity
- Remote firmware updates required (no physical access)
- Device expected to run unattended for long periods

These constraints directly influenced architectural decisions described below.

---

## High-Level Architecture

The firmware is organized around **explicit state management** rather than direct command execution.

High-level layers:

1. **Communication Layer**
   - Secure MQTT over TLS
   - JSON-based command and acknowledgment protocol
2. **State Management Layer**
   - Centralized system state representation
   - Persistent state stored in NVS
3. **Hardware Abstraction Layer**
   - GPIO, PWM, sensor, and actuator control
   - No direct hardware access from communication handlers
4. **System Supervision**
   - Crash detection
   - Safe-mode entry
   - OTA lifecycle control

This separation ensures that failures or invalid inputs never directly propagate to hardware behavior.

---

## RTOS Design (FreeRTOS)

FreeRTOS is used to explicitly control concurrency and timing behavior.

Design principles:
- Tasks have clearly defined responsibilities
- Communication callbacks never directly manipulate hardware
- Inter-task communication uses queues and events to avoid blocking behavior
- System state transitions are serialized to prevent race conditions

This structure allows predictable behavior even under high message load or partial system failure.

---

## Communication Architecture

### MQTT over TLS

MQTT was selected due to:
- Suitability for unreliable networks
- Broker-mediated decoupling between device and backend
- Support for retained messages and QoS

TLS is mandatory to ensure secure communication in untrusted networks.

### Command Handling Philosophy

Incoming MQTT commands:
- Are validated and parsed
- Update desired system state
- Never directly trigger hardware actions

The actual hardware behavior is driven exclusively by the **state engine**, ensuring consistency across reboots and reconnects.

---

## State-Driven Design

The firmware is explicitly **state-driven**, not command-driven.

Rationale:
- Commands may be duplicated, delayed, or arrive out of order
- Device state must remain authoritative
- Hardware behavior must be reproducible after reboot

State is:
- Persisted in NVS
- Restored on boot
- Compared against desired state from backend

This approach ensures deterministic outcomes regardless of communication anomalies.

---

## OTA Update Strategy

OTA updates are treated as a **high-risk operation** and are therefore tightly controlled.

Key aspects:
- Partition-based OTA using ESP-IDF mechanisms
- SHA-256 firmware integrity verification
- Automatic rollback on boot failure
- Crash counter used to detect unstable firmware versions

The device always prefers **known-good firmware** over attempting to boot potentially corrupted updates.

---

## Failure Detection and Safe Mode

The system continuously monitors its own stability.

Failure handling strategy:
- Crash count stored persistently
- Repeated failures trigger safe mode
- Safe mode disables non-essential functionality
- Device remains reachable for diagnostics or recovery OTA

This ensures that a faulty deployment does not permanently brick devices in the field.

---

## Hardware Interaction Philosophy

Hardware control is intentionally isolated from:
- MQTT callbacks
- OTA handlers
- Error recovery logic

All hardware actions are executed based on validated system state.
This minimizes the risk of undefined hardware behavior due to malformed input or partial system initialization.

---

## Trade-offs and Explicit Non-Goals

To maintain system robustness, several decisions were made intentionally:

- No dynamic memory-heavy abstractions
- No direct cloud-to-hardware command execution
- Limited feature set in favor of predictable behavior
- Preference for explicit logic over hidden framework behavior

These trade-offs favor reliability and maintainability over rapid feature expansion.

---

## Production Readiness Focus

This firmware is designed with the assumption that:
- Devices will fail
- Networks will be unreliable
- Updates will occasionally go wrong

The architecture is built to **expect failure and recover from it autonomously**, which is essential for real-world IoT deployments.
