# Wearable Embedded Project

### Group Members
**Marco Mattiuz**, **Luca Gojie Zhan**, **Giorgio Marasca**, **Francesco Buscardo**

## Introduction
The goal of this project is to design and implement a low-cost, open-source wearable device using widely available and easily upgradeable components.

We focused on features that are essential for modern wearables, such as:

- Heart rate monitoring  
- Step counting  
- Bluetooth Low Energy (BLE) communication with a web app  
- An OLED display to show real-time information  

To ensure maintainability and accessibility for anyone interested in experimenting or extending the system, we built the device on the **ESP32 platform** using **ESP-IDF**, chosen for its conectivity (BLE/wifi), flexibility, performance, and strong community support.

## Components
- **ESP32 board**
- **MAX30102 ppg sensor**, (works with every MAX3010x sensor)
- **MPU6050 mpu**
- **SH1106 oled diplay**
- **3.7V lithium battery**


## Team Contributions

| Member | Contributions |
|--------|---------------|
| **Marco Mattiuz** | developed a library for the MAX30102 sensor using I2C protocol. The library also includes signal processing to calculate heart rate. |
| **Luca Gojie Zhan** | developed a library for the sh1106 oled monitor. Wrote the code to handle button and wrist rotation events (the button uses interrupts) to change the state of the display and to turn it off.|
| **Giorgio Marasca** | worked on ble connectivity and web application |
| **Francesco Buscardo** | developed a library for MPU6050 sensor using I2C protocol. The library also calculates step count and detects wrist rotation|