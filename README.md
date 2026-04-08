# 🏍️ Smart Helmet - IoT Accident Detection System

[![ESP32](https://img.shields.io/badge/ESP32-WROOM--32-blue)](https://www.espressif.com/)
[![Arduino](https://img.shields.io/badge/Arduino-IDE-green)](https://www.arduino.cc/)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)
[![Telegram](https://img.shields.io/badge/Telegram-Bot-blue)](https://telegram.org/)

&gt; **Real-time accident detection and emergency alert system for motorcyclists using ESP32, ADXL345 accelerometer, and GPS. Sends SOS alerts with location via Telegram Bot API.**

**🎓 MSc Electronics Project | 🏢 Internship at Zenith Tek | 📍 Osmania University**

![Smart Helmet System](images/helmet_front.jpg)

---

## 📋 Table of Contents
- [Project Overview](#project-overview)
- [Key Features](#key-features)
- [System Architecture](#system-architecture)
- [Hardware Components](#hardware-components)
- [Circuit Design](#circuit-design)
- [Software Implementation](#software-implementation)
- [Installation Guide](#installation-guide)
- [Results & Testing](#results--testing)
- [Future Scope](#future-scope)
- [Documentation](#documentation)
- [Author](#author)

---

## 🎯 Project Overview

This project was developed as part of my **MSc Electronics degree** at **Wesley PG College, Osmania University** and completed during my **internship at Zenith Tek** (May 2024 - August 2024).

### Problem Statement
Road accidents involving two-wheelers often result in fatalities due to delayed emergency response. Victims may be unconscious or unable to call for help. This Smart Helmet automatically detects accidents and alerts emergency contacts with precise GPS location.

### Solution
An intelligent IoT-enabled helmet that:
- **Detects** accidents using accelerometer (free-fall & impact detection)
- **Locates** precise GPS coordinates of the incident
- **Alerts** emergency contacts via Telegram within 5 seconds
- **Monitors** battery life and system health remotely

**Impact**: Reduces emergency response time from minutes to seconds, potentially saving lives.

---

## ✨ Key Features

| Feature | Implementation | Status |
|---------|---------------|--------|
| 🚨 **Accident Detection** | ADXL345 3-axis accelerometer with FIFO trigger mode | ✅ Working |
| 📍 **GPS Tracking** | NEO-6M module with 2.5m accuracy | ✅ Working |
| 💬 **Instant Alerts** | Telegram Bot API with Google Maps integration | ✅ Working |
| 🔋 **Battery Management** | 18650 Li-ion with TP4056 charging + voltage monitoring | ✅ Working |
| 🔄 **Two-way Communication** | Telegram commands: location, status, battery, info | ✅ Working |
| 🛡️ **Fault Tolerance** | Watchdog timer, heap monitoring, auto-restart | ✅ Working |
| 💡 **Visual Feedback** | LED indicators for WiFi, activity, alerts | ✅ Working |

### Telegram Bot Commands
- `location` - Get current GPS coordinates with map link
- `status` - System status (WiFi, IP, battery)
- `battery` - Battery percentage
- `voltage` - Raw battery voltage
- `info` - Complete system diagnostics
- `restart` - Remote system reboot

---

## 🏗️ System Architecture

### Block Diagram
![Block Diagram](hardware/block_diagram.png)

**Data Flow:**
1. **Sensors** (ADXL345 + GPS) → **ESP32** (Processing)
2. **ESP32** → **Telegram Server** (HTTPS/JSON)
3. **Telegram Server** → **Emergency Contacts** (Instant Message)

### Pin Configuration
| Component | ESP32 Pin | Protocol |
|-----------|-----------|----------|
| ADXL345 SDA | GPIO21 | I2C |
| ADXL345 SCL | GPIO22 | I2C |
| ADXL345 INT | GPIO19 | Digital Interrupt |
| GPS TX | GPIO16 | UART2 RX |
| GPS RX | GPIO17 | UART2 TX |
| Battery Monitor | GPIO34 | ADC1_CH0 |
| Status LED | GPIO2 | Digital Output |

---

## 🔧 Hardware Components

| Component | Model | Specifications | Purpose |
|-----------|-------|---------------|---------|
| **Microcontroller** | ESP32-WROOM-32 | Dual-core 240MHz, WiFi/BT | Main processor & communication |
| **Accelerometer** | ADXL345 | ±16g, 13-bit, I2C/SPI | Impact & free-fall detection |
| **GPS Module** | NEO-6M | 2.5m accuracy, 50 channels | Location tracking |
| **Battery** | 18650 Li-ion | 3.7V, 2600mAh | Power supply |
| **Charging Module** | TP4056 | CC/CV, 1A max, protection | Battery charging & management |
| **Antenna** | Ceramic Patch | 1575.42 MHz | GPS signal reception |

**Total System Cost**: ~₹2,500 ($30 USD)

### Power Consumption
- **Active Mode**: ~120mA (WiFi + GPS + Sensors)
- **Deep Sleep**: &lt;10μA (ULP co-processor monitoring)
- **Battery Life**: 8+ hours continuous operation

---

## 🔌 Circuit Design

### Complete Circuit Diagram
![Circuit Diagram](hardware/circuit_diagram.png)

### Wiring Details

**ADXL345 Accelerometer (I2C):**
