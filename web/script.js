const SERVICE_UUID = 0x1847;
const TIME_CHAR_UUID = 0x2a2b;
const FLOAT32_CHAR_UUID = 0x0014;
const GYRO_CHAR_UUID = 0x0015;
const BPM_CHAR_UUID = 0x0016;
const AVGBPM_CHAR_UUID = 0x0017;
const DEVICE_NAME = "ESP32_BLE";

let bluetoothDevice = null;
let timeCharacteristic = null;
let float32Characteristic = null;
let gyroCharacteristic = null;
let int16Charateristic = null;
let uint32Charateristic = null;

const statusDiv = document.getElementById("status");
const connectBtn = document.getElementById("connectBtn");
const logDiv = document.getElementById("log");
const DataDiv = document.getElementById("data");

/* data */
window.IRACsampleArr = [];
window.BPMsampleArr = [];
window.AVGBPMsampleArr = [];
let MAX_SIZE_IRAC_BUFFER = 960;
let MAX_SIZE_BPM_BUFFER = 200;


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
    float32Characteristic = await service.getCharacteristic(FLOAT32_CHAR_UUID);
    gyroCharacteristic = await service.getCharacteristic(GYRO_CHAR_UUID);
    int16Charateristic = await service.getCharacteristic(BPM_CHAR_UUID);
    uint32Charateristic = await service.getCharacteristic(AVGBPM_CHAR_UUID);

    await float32Characteristic.startNotifications();
    float32Characteristic.addEventListener(
      "characteristicvaluechanged",
      handleNotification
    );

    await gyroCharacteristic.startNotifications();
    gyroCharacteristic.addEventListener(
      "characteristicvaluechanged",
      handleGyroNotification
    );

    await int16Charateristic.startNotifications();
    int16Charateristic.addEventListener(
      "characteristicvaluechanged",
      handleBPM
    );

    await uint32Charateristic.startNotifications();
    uint32Charateristic.addEventListener(
      "characteristicvaluechanged",
      handleAVGBPM
    );

    await sendTimeValue(Date.now());

    log("Connected successfully!", "success");
    updateUI(true);
  } catch (error) {
    log(`Error: ${error}`, "error");
    console.error(error);
  }
}

function onDisconnected() {
  log("Device disconnected", "error");
  updateUI(false);
  timeCharacteristic = null;
  float32Characteristic = null;
  gyroCharacteristic = null;
}

function handleGyroNotification(event) {
  const value = event.target.value;
  if (value.byteLength >= 6) {
    const gx = value.getInt16(0, true);
    const gy = value.getInt16(2, true);
    const gz = value.getInt16(4, true);
    log("Gyro: X=${gx} Y=${gy} Z=${gz}");
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

// function handleNotification(event) {
//     const value = event.target.value;
//     const float32Value = value.getFloat32(0, true);

//     DataDiv.innerHTML = `
//         <strong>Float32:</strong> ${float32Value.toFixed(2)}<br>
//         <strong>Timestamp:</strong> ${new Date().toLocaleTimeString()}
//     `;

//     log(`Float32: ${float32Value.toFixed(2)}`, 'success');
// }

function updateDropdown(bpm, avg) {
    document.getElementById("dropdown-bpm").textContent = `BPM: ${bpm}`;
    document.getElementById("dropdown-avg").textContent = `AVG BPM: ${avg}`;
}

function handleBPM(event) {
  const value = event.target.value;
  window.BPMsampleArr.push(value.getInt16(0, true));
  log("BPM: " + window.BPMsampleArr);
  if (window.BPMsampleArr.length >= MAX_SIZE_BPM_BUFFER) {
    window.BPMsampleArr = [];
  }
  updateDropdown(BPMsampleArr.at(-1),AVGBPMsampleArr.at(-1));
  updateBPMGraph();
}

function handleAVGBPM(event) {
  const value = event.target.value;
  window.AVGBPMsampleArr.push(value.getInt16(0, true));
  log("AVG_BPM: " + window.AVGBPMsampleArr);
  if (window.AVGBPMsampleArr.length >= MAX_SIZE_BPM_BUFFER) {
    window.AVGBPMsampleArr = [];
  }
  updateDropdown(BPMsampleArr.at(-1),AVGBPMsampleArr.at(-1));
  updateBPMGraph();
}

function handleNotification(event) {
  const value = event.target.value;
  // 16bit = 2 byte
  for (let i = 0; i < value.byteLength; i += 2) {
    window.IRACsampleArr.push(value.getInt16(i, true));
  }
  if (window.IRACsampleArr.length >= MAX_SIZE_IRAC_BUFFER) {
    window.IRACsampleArr = [];
  }

  updateGraphs();
  console.log(`Array int16: [${window.IRACsampleArr.join(', ')}]`, 'success');
}



connectBtn.addEventListener("click", toggleConnection);

// Check if Web Bluetooth is supported
if (!navigator.bluetooth) {
  log("Web Bluetooth API is not available in this browser!", "error");
  connectBtn.disabled = true;
  statusDiv.textContent = "Browser not supported";
  statusDiv.className = "status disconnected";
}
