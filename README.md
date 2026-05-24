# Air Radar System 🛰️

A real-time embedded air radar system that detects object distances, processes the data on-chip, and visualizes targets graphically. This project showcases full-stack embedded engineering—bridging bare-metal C firmware with a high-level Python graphical user interface (GUI).

---

## 🚀 System Architecture

The system operates in a three-stage pipeline to capture and display environmental data:
1. **Data Acquisition:** The ultrasonic sensor scans the environment and transmits raw timing data to the microcontroller over the **I2C** protocol.
2. **Embedded Processing:** The **PIC16F877A** firmware calculates the precise distance of objects based on the sensor signal, formatting the data packets for transmission.
3. **Real-Time Visualization:** Data is sent via serial communication to a laptop, where a **Python** script processes the stream and renders a live, dynamic radar-style display.

---

## 🛠️ Tech Stack & Components

* **Microcontroller:** Microchip PIC16F877A
* **Sensors:** Ultrasonic Distance Sensor
* **Communication Protocols:** I2C (Sensor-to-MCU), UART/Serial (MCU-to-PC)
* **Languages:** C (Firmware), Python (Visualization)

---

## ⚡ Key Engineering Features Demonstrated

* **Hardware Peripheral Configuration:** Hands-on implementation of I2C communication and hardware timers on an 8-bit MCU.
* **Serial Data Protocol:** Designed a robust data packet structure to transfer real-time coordinate data without lag or corruption.
* **Cross-Platform Integration:** Bridged low-level hardware constraints with high-level software visualization tools.
