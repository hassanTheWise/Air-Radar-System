# Air Radar System 🛰️

A real-time embedded air radar system that detects object distances, processes the data on-chip, displays local telemetry on an I2C-backed LCD, and visualizes targets graphically on a PC. This project showcases full-stack embedded engineering-bridging bare-metal C firmware with a high-level Python graphical user interface (GUI).

---

## 🚀 System Architecture

The system operates in a multi-stage pipeline to capture, display, and transmit environmental data over shared communication buses:
1. **Data Acquisition & Sweep:** A servo motor rotates the ultrasonic sensor to scan a 180-degree field. The sensor transmits raw timing data to the microcontroller over the shared **I2C** bus.
2. **Embedded Processing & Shared Bus Display:** The **PIC16F877A** firmware controls the servo timing via PWM and calculates target distances. To optimize GPIO pin usage, it outputs this telemetry data locally to an **LCD Screen sharing the same I2C bus** via an I2C expander module.
3. **Real-Time Visualization:** Simultaneously, coordinate data is sent via UART through a **USB-to-TTL converter** to a laptop, where a **Python** script processes the serial stream and renders a live dynamic radar-style display.

---

## 🛠️ Tech Stack & Components

* **Microcontroller:** Microchip PIC16F877A
* **Actuator:** Servo Motor (for 180° radar sweep control via PWM)
* **Sensors:** Ultrasonic Distance Sensor
* **Displays:** 16x2 LCD Screen with I2C Backpack Module (for pin-optimized local telemetry)
* **PC Interface:** USB-to-TTL Converter (for UART serial data transmission)
* **Communication Protocols:** I2C (Shared bus for Sensor and LCD), UART/Serial (MCU-to-PC)
* **Languages:** C (Firmware), Python (Visualization)

---

## ⚡ Key Engineering Features Demonstrated

* **I2C Bus Multiplexing:** Implemented a shared I2C communication bus to read from sensors and write to a display simultaneously, significantly reducing required GPIO pin count.
* **Hardware Peripheral Configuration:** Hands-on implementation of I2C communication, hardware timers, and PWM signal generation for servo motor control on an 8-bit MCU.
* **Serial Communication & Interfacing:** Leveraged a USB-to-TTL converter to bridge microchip UART pins with PC serial ports using a robust data packet structure.
* **Cross-Platform Integration:** Bridged low-level hardware constraints with high-level software visualization tools.
