<div align="center">

# 🛡️ ESP32 WiFi Sentinel

**A portable WiFi security analyzer & Evil Twin detector**

[![Arduino](https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![ESP32](https://img.shields.io/badge/ESP32-E7352C?style=for-the-badge&logo=espressif&logoColor=white)](https://www.espressif.com/)
[![License: CC BY-NC 4.0](https://img.shields.io/badge/License-CC%20BY--NC%204.0-lightgrey.svg?style=for-the-badge)](https://creativecommons.org/licenses/by-nc/4.0/)
[![Status](https://img.shields.io/badge/Status-Active-success?style=for-the-badge)]()

</div>

---

ESP32 WiFi Sentinel is a handheld cybersecurity tool that scans nearby WiFi networks in real-time, calculates environment risk scores, and detects potential **Evil Twin** attacks — all displayed on a compact OLED screen and controlled with a rotary encoder.

<div align="center">
  <img width="320" height="300" alt="Main Menu" src="https://github.com/user-attachments/assets/ab12a8b9-39b4-49e3-9312-646a84041f94" />

</div>

## ⚡ Key Features

| Feature | Description |
| :--- | :--- |
| 📡 **WiFi Radar** | Real-time scanning with AP count, open network detection & signal analysis |
| 🔴 **Risk Scoring** | Dynamic 0–100 risk score based on open networks, density & clone presence |
| 🕵️ **Evil Twin Detection** | Identifies cloned SSIDs and flags suspicious RSSI anomalies |
| ⚙️ **Configurable** | Adjustable risk sensitivity, clone aggressiveness & scan intervals |

## 🔧 Hardware

- ESP32 Dev Module
- SSD1306 OLED Display (128×64, I2C)
- Rotary Encoder (KY-040 or similar)

## 🚀 Getting Started

1. Install the following libraries via Arduino Library Manager:
   - `Adafruit GFX Library`
   - `Adafruit SSD1306`
   - `AiEsp32RotaryEncoder`

2. Open `wifisentinel.ino` in Arduino IDE

3. Select your ESP32 board & port, then upload

## 🕹️ Controls

- **Rotate** → Navigate menus
- **Press** → Select / Go back

<div align="left">

**Berkkan KAYA**

[![GitHub](https://img.shields.io/badge/GitHub-kayaberkkan-181717?style=for-the-badge&logo=github)](https://github.com/kayaberkkan)

</div>

## 📄 License

This project is licensed under the [Creative Commons Attribution-NonCommercial 4.0 International License](https://creativecommons.org/licenses/by-nc/4.0/).  
You are free to use, share, and modify — but **not for commercial purposes**.
