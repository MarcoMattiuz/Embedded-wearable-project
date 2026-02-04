const SERVICE_UUID = 0x1847;
const TIME_CHAR_UUID = 0x2a2b;
const IRACBUFFER_CHAR_UUID = 0x0014;
const GYRO_CHAR_UUID = 0x0015;
const BPM_CHAR_UUID = 0x0016;
const AVGBPM_CHAR_UUID = 0x0017;
const IRRAWBUFFER_CHAR_UUID = 0x0018;
const WEATHER_CHAR_UUID = 0x0019;
const ENS160_CHAR_UUID = 0x0029;

const DEVICE_NAME = "ESP32_BLE";

let bluetoothDevice = null;
let timeCharacteristic = null;
let iracbufferCharacteristic = null;
let gyroCharacteristic = null;
let bpmCharateristic = null;
let avgbpmCharateristic = null;
let float32Characteristic = null;
let weatherCharacteristic = null;

let latitude = 0.0;
let longitude = 0.0;
const WeatherType = {
  SUNNY: 0,
  CLOUDY: 1,
  RAINY: 2,
  FOGGY: 3,
  SNOWY: 4,
  THUNDERSTORM: 5,
};
let ens160Characteristic = null;

const statusDiv = document.getElementById("status");
const connectBtn = document.getElementById("connectBtn");
const logDiv = document.getElementById("log");
const DataDiv = document.getElementById("data");

/* data */
window.IRACsampleArr = [];
window.IRRAWsampleArr = [];
window.BPMsampleArr = [];
window.AVGBPMsampleArr = [];
window.ECO2sampleArr = [];
let MAX_SIZE_IRAC_BUFFER = 960;
let MAX_SIZE_BPM_BUFFER = 200;
let MAX_SIZE_IRRAW_BUFFER = 960;
let MAX_SIZE_ECO2_BUFFER = 200;

function log(message, type = "info") {
  const entry = document.createElement("div");
  entry.className = `log-entry ${type}`;
  entry.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
  logDiv.appendChild(entry);
  logDiv.scrollTop = logDiv.scrollHeight;
}

function updateUI(connected) {
  statusDiv.textContent = connected
    ? `Connected to ${DEVICE_NAME}`
    : "Disconnected";
  statusDiv.className = `status ${connected ? "connected" : "disconnected"}`;
  connectBtn.textContent = connected ? "Disconnect" : "Connect to ESP32";
  connectBtn.className = connected ? "btn-disconnect" : "btn-connect";
}

async function toggleConnection() {
  if (bluetoothDevice?.gatt.connected) {
    bluetoothDevice.gatt.disconnect();
    log("Disconnected by user", "info");
    return;
  }

  try {
    log("Connecting to BLE device...", "info");
    bluetoothDevice = await navigator.bluetooth.requestDevice({
      filters: [{ name: DEVICE_NAME }],
      optionalServices: [SERVICE_UUID],
    });

    bluetoothDevice.addEventListener("gattserverdisconnected", onDisconnected);
    const server = await bluetoothDevice.gatt.connect();

    const service = await server.getPrimaryService(SERVICE_UUID);
    timeCharacteristic = await service.getCharacteristic(TIME_CHAR_UUID);
    iracbufferCharacteristic =
      await service.getCharacteristic(IRACBUFFER_CHAR_UUID);
    irrawbufferCharacteristic = await service.getCharacteristic(
      IRRAWBUFFER_CHAR_UUID,
    );
    ens160Characteristic = await service.getCharacteristic(ENS160_CHAR_UUID);
    gyroCharacteristic = await service.getCharacteristic(GYRO_CHAR_UUID);
    bpmCharateristic = await service.getCharacteristic(BPM_CHAR_UUID);
    avgbpmCharateristic = await service.getCharacteristic(AVGBPM_CHAR_UUID);
    weatherCharacteristic = await service.getCharacteristic(WEATHER_CHAR_UUID);

    await iracbufferCharacteristic.startNotifications();
    iracbufferCharacteristic.addEventListener(
      "characteristicvaluechanged",
      handleIRACbuffer,
    );

    await irrawbufferCharacteristic.startNotifications();
    irrawbufferCharacteristic.addEventListener(
      "characteristicvaluechanged",
      handleIRRAWbuffer,
    );

    await ens160Characteristic.startNotifications();
    ens160Characteristic.addEventListener(
      "characteristicvaluechanged",
      handleEns160Notification,
    );

    await gyroCharacteristic.startNotifications();
    gyroCharacteristic.addEventListener(
      "characteristicvaluechanged",
      handleGyroNotification,
    );

    await bpmCharateristic.startNotifications();
    bpmCharateristic.addEventListener("characteristicvaluechanged", handleBPM);

    await avgbpmCharateristic.startNotifications();
    avgbpmCharateristic.addEventListener(
      "characteristicvaluechanged",
      handleAVGBPM,
    );

    await sendTimeValue(Date.now());

    log("Connected successfully!", "success");
    updateUI(true);

    await getWeather();
    log("Weather data sent to device", "success");
  } catch (error) {
    log(`Error: ${error}`, "error");
    console.error(error);
  }
}

function onDisconnected() {
  log("Device disconnected", "error");
  updateUI(false);
  timeCharacteristic = null;
  iracbufferCharacteristic = null;
  gyroCharacteristic = null;
  ens160Characteristic = null;
  weatherCharacteristic = null;
}

function handleEns160Notification(event) {
  const value = event.target.value;

  if (value.byteLength >= 5) {
    const eco2 = value.getUint16(0, true);
    const tvoc = value.getUint16(2, true);
    const aqi = value.getUint8(4);

    const now = new Date();
    const timestamp =
      now.getHours().toString().padStart(2, "0") +
      ":" +
      now.getMinutes().toString().padStart(2, "0") +
      ":" +
      now.getSeconds().toString().padStart(2, "0");

    window.ECO2sampleArr.push({
      value: eco2,
      timestamp: timestamp,
    });

    if (window.ECO2sampleArr.length >= MAX_SIZE_ECO2_BUFFER) {
      window.ECO2sampleArr = [];
    }

    updateECO2Graph();
    // log(`eCO2: ${eco2} ppm, TVOC: ${tvoc} ppb, AQI: ${aqi}`);
  }
}

function handleGyroNotification(event) {
  const value = event.target.value;

  if (value.byteLength >= 4) {
    const roll = value.getFloat32(0, true);//roll
    const pitch = value.getFloat32(4, true);//pitch
    

    if (typeof window.update3DObject === "function") {
      window.update3DObject(roll,pitch);
    }

    console.log(
      `GYRO - roll: ${roll.toFixed(3)}, pitch: ${pitch.toFixed(3)}}`
    );
  }
}

async function sendTimeValue(timestamp) {
  if (!timeCharacteristic) {
    log("Not connected!", "error");
    return;
  }

  try {
    const date = new Date(timestamp);
    const bleDay = date.getDay() || 7; // Convert Sunday from 0 to 7
    const fractions256 = Math.floor((date.getMilliseconds() / 1000) * 256);

    const buffer = new ArrayBuffer(10);
    const view = new DataView(buffer);

    view.setUint16(0, date.getFullYear(), true);
    view.setUint8(2, date.getMonth() + 1);
    view.setUint8(3, date.getDate());
    view.setUint8(4, date.getHours());
    view.setUint8(5, date.getMinutes());
    view.setUint8(6, date.getSeconds());
    view.setUint8(7, bleDay);
    view.setUint8(8, fractions256);
    view.setUint8(9, 0);

    await timeCharacteristic.writeValue(buffer);
    log(`Time sent to ESP32: ${date.toLocaleString()}`, "success");
  } catch (error) {
    log(`Write error: ${error}`, "error");
  }
}

function updateDropdown(bpm, avg) {
  document.getElementById("dropdown-bpm").textContent = `BPM: ${bpm}`;
  document.getElementById("dropdown-avg").textContent = `AVG(4) BPM: ${avg}`;
  document.getElementById("dropdown-total-avg").textContent =
    `TOTAL AVG BPM: ${Math.round(window.AVGBPMsampleArr.reduce((a, b) => a + b.value, 0) / window.AVGBPMsampleArr.length)}`;
}

function handleBPM(event) {
  const value = event.target.value;
  const now = new Date();
  const timestamp =
    now.getHours().toString().padStart(2, "0") +
    ":" +
    now.getMinutes().toString().padStart(2, "0");
  window.BPMsampleArr.push({
    value: value.getInt16(0, true),
    timestamp: timestamp,
  });
  // log("BPM: " + window.BPMsampleArr.map((s) => s.value).join(", "));
  if (window.BPMsampleArr.length >= MAX_SIZE_BPM_BUFFER) {
    window.BPMsampleArr = [];
  }
  if (BPMsampleArr.length > 0 && AVGBPMsampleArr.length > 0) {
    updateDropdown(BPMsampleArr.at(-1).value, AVGBPMsampleArr.at(-1).value);
  }
  updateBPMGraph();
}

function handleAVGBPM(event) {
  const value = event.target.value;
  const now = new Date();
  const timestamp =
    now.getHours().toString().padStart(2, "0") +
    ":" +
    now.getMinutes().toString().padStart(2, "0");
  window.AVGBPMsampleArr.push({
    value: value.getInt16(0, true),
    timestamp: timestamp,
  });
  // log("AVG_BPM: " + window.AVGBPMsampleArr.map((s) => s.value).join(", "));
  if (window.AVGBPMsampleArr.length >= MAX_SIZE_BPM_BUFFER) {
    window.AVGBPMsampleArr = [];
  }
  if (BPMsampleArr.length > 0 && AVGBPMsampleArr.length > 0) {
    updateDropdown(BPMsampleArr.at(-1).value, AVGBPMsampleArr.at(-1).value);
  }
  updateBPMGraph();
}

function handleIRACbuffer(event) {
  const value = event.target.value;
  // 16bit = 2 byte
  for (let i = 0; i < value.byteLength; i += 2) {
    window.IRACsampleArr.push(value.getInt16(i, true));
  }
  if (window.IRACsampleArr.length >= MAX_SIZE_IRAC_BUFFER) {
    window.IRACsampleArr = [];
  }

  updateIRACGraphs();
  // console.log(
  //   `Array IR AC int16: [${window.IRACsampleArr.join(", ")}]`,
  //   "success",
  // );
}

function handleIRRAWbuffer(event) {
  const value = event.target.value;
  // 32bit = 4 byte
  for (let i = 0; i < value.byteLength; i += 4) {
    if (i + 4 <= value.byteLength) {
      window.IRRAWsampleArr.push(value.getUint32(i, true));
    }
  }
  if (window.IRRAWsampleArr.length >= MAX_SIZE_IRRAW_BUFFER) {
    window.IRRAWsampleArr = [];
  }

  updateIRRAWGraphs();
  // console.log(
  //   `Array IR RAW Uint32: [${window.IRRAWsampleArr.join(", ")}]`,
  //   "success",
  // );
}

//convert weather code from api to a WeatherType
function mapWeatherCode(code) {
  switch (code) {
    // CLEAR
    case 0:
      return WeatherType.SUNNY;

    // CLOUDY
    case 1:
    case 2:
    case 3:
      return WeatherType.CLOUDY;

    // FOG
    case 45:
    case 48:
      return WeatherType.FOGGY;

    // DRIZZLE or RAIN
    case 51:
    case 53:
    case 55:
    case 56:
    case 57:
    case 61:
    case 63:
    case 65:
    case 66:
    case 67:
    case 80:
    case 81:
    case 82:
      return WeatherType.RAINY;

    // SNOW
    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86:
      return WeatherType.SNOWY;

    // THUNDERSTORM
    case 95:
    case 96:
    case 99:
      return WeatherType.THUNDERSTORM;

    default:
      return WeatherType.CLOUDY;
  }
}

async function sendWeatherData(temp, weatherType) {
  if (!weatherCharacteristic) {
    log("Weather characteristic not available!", "error");
    return;
  }

  try {
    const buffer = new ArrayBuffer(5);
    const view = new DataView(buffer);
    view.setFloat32(0, temp, true); // float temperature
    view.setUint8(4, weatherType); // int weather type

    await weatherCharacteristic.writeValue(buffer);
    log(`Weather sent: temp=${temp}, type=${weatherType}`, "success");
  } catch (error) {
    log(`Error sending weather: ${error}`, "error");
  }
}

function getGeolocation() {
  return new Promise((resolve, reject) => {
    if (!navigator.geolocation) {
      log("Geolocation is not supported by this browser");
      reject(new Error("Geolocation unsupported"));
      return;
    }

    navigator.geolocation.getCurrentPosition(
      (position) => {
        resolve({
          latitude: position.coords.latitude,
          longitude: position.coords.longitude,
        });
      },
      (error) => {
        log("Sorry, no position available.");
        reject(error);
      },
    );
  });
}

async function getWeather() {
  try {
    const { latitude, longitude } = await getGeolocation();

    const URL =
      `https://api.open-meteo.com/v1/forecast` +
      `?latitude=${latitude}&longitude=${longitude}` +
      `&current=weather_code,temperature_2m`;

    console.log("URL:", URL);

    const r = await fetch(URL);
    const data = await r.json();

    const weatherCode = data.current.weather_code;
    const code = mapWeatherCode(weatherCode);
    const temp = data.current.temperature_2m;

    console.log("code:", code, "temp:", temp);

    sendWeatherData(temp, code);
  } catch (error) {
    console.error(error);
    log(error, "error");
  }
}

connectBtn.addEventListener("click", toggleConnection);

// Check if Web Bluetooth is supported
if (!navigator.bluetooth) {
  log("Web Bluetooth API is not available in this browser!", "error");
  connectBtn.disabled = true;
  statusDiv.textContent = "Browser not supported";
  statusDiv.className = "status disconnected";
}
