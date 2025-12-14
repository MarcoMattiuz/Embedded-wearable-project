# Wearable Embedded Project

### Group Members
**Marco Mattiuz**, **Luca Guojie Zhan**, **Giorgio Marasca**, **Francesco Buscardo**

## Introduction and Features
The goal of this project is to design and implement a low-cost, open-source wearable device using widely available and easily upgradeable components.

We focused on features that are essential for modern wearables, such as:

- Heart rate monitoring  
- TODO: Sp02 measurement
- digital clock (updated using the web-app through BLE connectivity)
- Step counting  
- wrist rotation (to toggle the display)
- Bluetooth Low Energy (BLE) communication with a web-app 
- Weather data (using API) 
- An OLED display to show real-time information  
- Data plots in the web-app
- TODO: implementing C02/temperature/umidity sensor
- TODO: 3D model that follows device rotation (in the web-app)

To ensure maintainability and accessibility for anyone interested in experimenting or extending the system, we built the device on the **ESP32 platform** using **ESP-IDF**, chosen for its conectivity (BLE/wifi), flexibility, performance, and strong community support.

## Team Contributions
| Member | Contributions |
|--------|---------------|
| **Marco Mattiuz** | developed a library for the MAX30102 sensor using I2C protocol. The library also includes signal processing to calculate heart rate. Created the graphs in the web app using (plotly.js). Wrote some of the BLE connectivity |
| **Luca Guojie Zhan** | developed a library for the sh1106 oled monitor using I2C protocol. Wrote the code to handle button and wrist rotation events (the button uses interrupts) to change the state of the display and to turn it off. Implemented weather API in the web app.|
| **Giorgio Marasca** | developed BLE connectivity in esp32 board and web application. TODO: Developed a library for the C02 sensor using I2C protocol.|
| **Francesco Buscardo** | developed a library for MPU6050 sensor using I2C protocol. The library also calculates step count and detects wrist rotation. TODO: 3D orientation visualization. (with three.js)|

## Components
- **ESP32 board**
- **MAX30102 ppg sensor**, (works with every MAX3010x sensor)
- **MPU6050 mpu**
- **SH1106 oled diplay**
- TODO: **C02 sensor**
- Voltage step-up
- **3.7V lithium battery**

## Project structure

## Project structure

```ascii
EmbeddedProject/
├── .github/
│   └── workflows/
├── .pio/
│   └── build/ ...
├── lib/
│   ├── MAX30102/
│   │   └── max30102_api.c / .h
│   ├── MPU6050/
│   │   └── mpu6050_api.c / .h
│   ├── SHARED/
│   │   └── common_utils.c / .h
│   └── ... (other shared libraries)
├── src/
│   └── main.c
├── tools/
│   └── python_scripts.py
├── web/
│   ├── index.html
│   ├── graphs.js
│   ├── script.js
│   ├── style.css
│   └── ... (other assets)
├── .gitignore
├── platformio.ini
└── README.md
```

## Run / debug the project
- The project was developed using Platformio IDE (vscode extension) for its integrated tools. We suggets to use it to flash the code. 
- to debug the project we use the serial monitor and to activate the prints it is needed to add the following line in the platformio.ini file: 
```ini
build_flags =
    -DDEBUG
```
- To check if the data of the MAX30102 sensor was good we wanted so save the logs and check them later. To do so you just have to use this command: `pio device monitor -b 115200 | tee serialmonitor.log`.
- in the tools folder there is a python script that generates a graph of a sample taken by the sensor. The script already reads the serialmonitor.log file, to use it do this (for macOs and linux):
```bash
cd tools/pyHrGraphs
python3 -m venv venv
source venv/bin/activate
python3 graph.py
```
⚠️ the python script has **sps** hard coded in, so if you change it in the MAX30102 configuration keep in mind that you will have to change it also there.
