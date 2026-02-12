# Wearable Embedded Project
**Accademic year:** 2025-2026
**Course:** Introduction to Embedded systems and IoT [145996]

### Group Members
**Marco Mattiuz**, **Luca Guojie Zhan**, **Giorgio Marasca**, **Francesco Buscardo**

### Presentation and Video links
#### ([Video](https://youtu.be/ByliZmJKJFA)
#### ([Presentation](https://www.canva.com/design/DAHBCtbX71E/7lhA4Xd-Q39TREjt6f52yw/view?utm_content=DAHBCtbX71E&utm_campaign=designshare&utm_medium=link2&utm_source=uniquelinks&utlId=hd3142368d5))

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
- A display to show real-time information  
- Data plots in the web-app
- CO2/TVOC/Air-quality measurement (maybe not essential for wearables 🙂)
- TODO: implementing temperature/umidity sensor
- 3D model that follows device rotation (in the web-app) **quaternion implementation is needed**
- battery level measurement 

To ensure maintainability and accessibility for anyone interested in experimenting or extending the system, we built the device on the **ESP32 platform** using **ESP-IDF**, chosen for its conectivity (BLE/wifi), flexibility, performance, and strong community support.

## Project overview

### Hardware wiring

```ascii
                         ┌───────────────────────────┐
                         │        ESP32 DevKit       │
                         │                           │
  3.3V ──────────────────┤ 3V3                       │
  GND  ──────────────────┤ GND                       │
  SDA  ──────────────────┤ GPIO21  (I2C SDA)         │
  SCL  ──────────────────┤ GPIO22  (I2C SCL)         │
                         │                           │
  ADC node ──────────────┤ GPIO32 (ADC1 CH4)         │
  Button ────────────────┤ GPIO18                    │
                         │                           │
  Battery + ─────────────┤ VIN                       │
  Battery - ─────────────┤ GND                       │
                         └───────────────────────────┘


        I2C BUS (all devices in parallel: SDA=21, SCL=22, 3.3V, GND)
   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐
   │  MAX30102   │   │  MPU6050    │   │   ENS160    │   │  SH1106     │
   │ VCC ─ 3.3V  │   │ VCC ─ 3.3V  │   │ VCC ─ 3.3V  │   │ VCC ─ 3.3V  │
   │ GND ─ GND   │   │ GND ─ GND   │   │ GND ─ GND   │   │ GND ─ GND   │
   │ SDA ─ GPIO21│   │ SDA ─ GPIO21│   │ SDA ─ GPIO21│   │ SDA ─ GPIO21│
   │ SCL ─ GPIO22│   │ SCL ─ GPIO22│   │ SCL ─ GPIO22│   │ SCL ─ GPIO22│
   └─────────────┘   └─────────────┘   └─────────────┘   └─────────────┘


        BATTERY + VOLTAGE DIVIDER (for ADC measurement on GPIO32)
   Battery +
      │
      ├───────────────> VIN (ESP32)
      │
     [R1 10kΩ]
      │───────────────> ADC node (Vout) ───────────> GPIO32 (ADC1)
     [R2 10kΩ]
      │
      └───────────────> GND (ESP32)  <────────────── Battery -


        BUTTON
   GPIO18 ────────[  push button  ]─────── GND
```

TODO: images...


### Project flow (user guide)

- Start the device by connecting the battery.
- The device runs a startup sequence where it initializes the sensors and the OLED display on the I2C bus and starts the related tasks. The ENS160 (CO2) task waits some time before starting. All other tasks can operate normally during this time.
- Once the loading screen ends, the device can connect to the ([web-app](https://marcomattiuz.github.io/Embedded-wearable-project/)). There, it is possible to view data plots from the MAX30102 and ENS160 sensors, as well as a 3D object that follows the movement of the MPU6050. Since quaternions were not implemented, the movement tracking is not perfectly precise.
- Using the button, you can switch between the different screens shown on the OLED display. A long press toggles the display on and off.



## Team Contributions

**Disclaimer:** Every member actively contributed to the project and is familiar with all its dynamics and inner workings. Some features were developed but later excluded during integration due to technical issues (ex. wrist rotation).

| Member | Contributions |
|--------|---------------|
| **Marco Mattiuz** | developed a library for the MAX30102 sensor using I2C protocol. The library also includes signal processing to calculate heart rate. Created the graphs in the web app using (plotly.js). Wrote some of the BLE connectivity functions. Created the task that uses the ADC to measures the voltage of the battery and estimate the charge state. Printed the 3D models 🙂|
| **Luca Guojie Zhan** | developed a library for the sh1106 oled monitor using I2C protocol. Created the screens. Wrote the code to handle button and wrist rotation events (the button uses interrupts) to change the state of the display and to turn it off. Implemented weather API in the web app and sent data with BLE.|
| **Giorgio Marasca** | developed BLE connectivity in esp32 board and web application. Developed a library for the C02 sensor using I2C protocol. Designed the 3D models|
| **Francesco Buscardo** | developed a library for MPU6050 sensor using I2C protocol. The library also calculates step count and detects wrist rotation. 3D orientation visualization (with three.js).|

## Components
- **ESP32 board**
- **MAX30102 ppg sensor**, (works with every MAX3010x sensor) (ppg -> Photoplethysmography)
- **MPU6050 mpu** (3-axis gyroscope and accellerometer)
- **SH1106 oled diplay**
- **Button**
- **ENS160 sensor** (CO2, particles...)
- **3.7V lithium battery**
- **2 10kΩ for the tension divider**

## Project structure

```ascii
EmbeddedProject/
├── .github/
│   ├── workflows/
│   │   └── deploy.yml
├── .pio/
│   └── build/ ...
├── lib/
│   ├── BLE/
│   │   ├── readme/
│   │   │   └──readme.md ...
│   │   └── ... .c / .h
│   ├── sh1160/
│   │   ├── readme/
│   │   │   └──readme.md ...
│   │   └── ... .c / .h
│   ├── MAX30102/
│   │   ├── readme/
│   │   │   └──readme.md ...
│   │   └── max30102_api.c / .h
│   ├── MPU6050/
│   │   ├── readme/
│   │   │   └──readme.md ...
│   │   └── mpu6050_api.c / .h
│   ├── ENS160/
│   │   ├── readme/
│   │   │   └──readme.md ...
│   │   └── ens160.c / .h
│   ├── SHARED/
│   │   └── common_utils.c / .h
│   └── ... (other)
├── src/
│   └── main.c
├── tools/
│   └── ...(python_scripts.py)
├── web/
│   ├── models/
│   ├── libs/
│   ├── index.html
│   ├── graphs.js
│   ├── script.js
│   ├── script3D.js
│   ├── style.css
│   └── ... (other files)
├── .gitignore
├── platformio.ini
└── README.md
```

## Run / debug the project
- The project was developed using Platformio IDE (vscode extension) for its integrated tools. We suggets to use it to flash the code. With Platformio CLI tools to compile and burn the code in the microcontroller use:
```bash
pio run -t upload
```
- To print we use a macro called **DBG_PRINTF()** that is enable only when the flag -DDEBUG.
- to debug the project we use the serial monitor and to activate some of the prints it is needed to add the following line (they might be commented out)  in the platformio.ini file:
```ini
build_flags =
    -DDEBUG
```
- To check if the data of the MAX30102 sensor was good we wanted so save the logs and check them later. To do so you just have to use this command (baud rate might be different): `pio device monitor -b 921600 | tee serialmonitor.log`.
- in the tools folder there is a python script that generates a graph of a sample taken by the sensor. The script already reads the serialmonitor.log file, to use it do this (for macOs and linux):

### create and install the venv
```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```
### use it
```bash
cd tools/pyHrGraphs
source venv/bin/activate
python3 graph.py
```
⚠️ the python script has **sps** hard coded in, so if you change it in the MAX30102 configuration keep in mind that you will have to change it also there.
- later in the project we added graphs in the web application, so now you can see there the raw and filtered data of the ppg sensor

### Web application deployment
The web app is automatically deployed via GitHub Actions whenever a commit is pushed to the master branch.

## Modify esp firmware

```bash
pio run -t menuconfig
```
