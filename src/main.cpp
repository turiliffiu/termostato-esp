#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>

// ========================================
// CONFIGURAZIONE WiFi
// ========================================
const char* ssid = "TuriMesh";
const char* password = "C1p0ll1na.Rav10la";

// ========================================
// CONFIGURAZIONE MQTT
// ========================================
const char* mqtt_server = "192.168.1.162";  // IP Home Assistant
const int mqtt_port = 1883;
const char* mqtt_user = "mqtt_user";
const char* mqtt_password = "salvo";
const char* mqtt_client_id = "wemos-termostato";
const char* mqtt_base_topic = "homeassistant";
const char* device_name = "Termostato Wemos";
const char* device_id = "termostato_wemos";

// ========================================
// CONFIGURAZIONE HARDWARE
// ========================================
// Relay
#define RELAY_PIN D4        // GPIO2

// Software I2C per AHT10 (usa RX/TX - no conflitti boot)
#define AHT_SDA 1         // GPIO1 (TX)
#define AHT_SCL 3          // GPIO3 (RX)

// LED integrato per debug visivo
#define LED_BUILTIN 2       // GPIO2 (condiviso con relay, lampeggia)

// ========================================
// SENSORI
// ========================================
TwoWire ahtWire;
Adafruit_AHTX0 aht;

// ========================================
// WEB SERVER & MQTT
// ========================================
AsyncWebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ========================================
// VARIABILI TERMOSTATO
// ========================================
// Temperatura
float currentTemp = 20.0;
float currentHumidity = 50.0;
float targetTemp = 21.0;
float hysteresis = 0.5;

// Stato
bool heatingOn = false;
bool systemEnabled = true;

// Modalità
enum OperatingMode {
    MODE_COMFORT,
    MODE_ECO,
    MODE_MANUAL
};
OperatingMode currentMode = MODE_COMFORT;

float comfortTemp = 21.0;
float ecoTemp = 18.0;

// Timing
unsigned long lastAHTRead = 0;
unsigned long lastMQTTPublish = 0;
unsigned long heatingOnTime = 0;
unsigned long lastHeatingChange = 0;

const unsigned long AHT_READ_INTERVAL = 5000;
const unsigned long MQTT_PUBLISH_INTERVAL = 30000;

// MQTT
bool mqttConfigSent = false;
bool ahtFound = false;

// ========================================
// FORWARD DECLARATIONS
// ========================================
void readAHT();
void updateHeating();
void publishMQTTState();
void publishMQTTDiscovery();
void reconnectMQTT();

// ========================================
// HTML DASHBOARD COMPLETA
// ========================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Termostato Wemos</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#ff6b6b,#ee5a6f);min-height:100vh;padding:20px}
.container{max-width:900px;margin:0 auto}
.card{background:rgba(255,255,255,0.95);border-radius:20px;padding:20px;margin-bottom:20px;box-shadow:0 10px 25px rgba(0,0,0,0.1)}
h1{color:#ff6b6b;font-size:2em;margin-bottom:10px}
h3{color:#333;font-size:1.2em;margin-bottom:15px}
.header-status{display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:10px}
.temp-display{text-align:center;margin:20px 0}
.temp-current{font-size:3.5em;font-weight:700;color:#ff6b6b}
.temp-control{display:flex;align-items:center;justify-content:center;gap:20px;margin:20px 0}
.temp-btn{width:50px;height:50px;border:none;border-radius:50%;background:#ff6b6b;color:white;font-size:1.5em;cursor:pointer;transition:transform 0.2s}
.temp-btn:hover{transform:scale(1.1)}
.temp-target{font-size:2em;font-weight:600;min-width:100px;text-align:center}
.sensor-row{display:flex;justify-content:space-between;padding:12px 0;border-bottom:1px solid #eee}
.sensor-row:last-child{border-bottom:none}
.badge{padding:6px 12px;border-radius:10px;font-size:0.9em;font-weight:600;display:inline-block}
.badge-heating{background:#fee;color:#c00}
.badge-idle{background:#dbeafe;color:#1e40af}
.badge-info{background:#e0e7ff;color:#3730a3;font-size:0.85em}
.mode-selector{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin:15px 0}
.mode-btn{padding:12px;border:2px solid #eee;border-radius:10px;background:white;cursor:pointer;font-weight:600;text-align:center;transition:all 0.3s}
.mode-btn:hover{border-color:#ff6b6b}
.mode-btn.active{background:#ff6b6b;color:white;border-color:#ff6b6b}
.toggle-switch{position:relative;width:60px;height:30px;background:#e5e7eb;border-radius:30px;cursor:pointer;transition:background 0.3s}
.toggle-switch.active{background:#ff6b6b}
.toggle-knob{position:absolute;top:3px;left:3px;width:24px;height:24px;background:white;border-radius:50%;transition:left 0.3s;box-shadow:0 2px 4px rgba(0,0,0,0.2)}
.toggle-switch.active .toggle-knob{left:33px}
.btn{width:100%;padding:14px;border:none;border-radius:10px;background:#6b7280;color:white;cursor:pointer;margin-top:10px;font-weight:600;font-size:1em;transition:background 0.3s}
.btn:hover{background:#4b5563}
.btn-primary{background:linear-gradient(135deg,#667eea,#764ba2);color:white}
.btn-primary:hover{opacity:0.9}
.info-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:15px;margin-top:15px}
.info-box{text-align:center;padding:15px;background:#f9fafb;border-radius:10px}
.info-label{font-size:0.85em;color:#6b7280;margin-bottom:5px}
.info-value{font-size:1.3em;font-weight:600;color:#1f2937}
</style>
</head>
<body>
<div class="container">

<!-- Header -->
<div class="card">
<div class="header-status">
<div>
<h1>🔥 Termostato Wemos</h1>
<span class="badge badge-info" id="ip-info">IP: ---</span>
</div>
<span class="badge" id="heating-badge">Idle</span>
</div>
</div>

<!-- Temperatura Corrente -->
<div class="card">
<h3>🌡️ Temperatura Ambiente</h3>
<div class="temp-display">
<span class="temp-current" id="current-temp">--</span>
<span style="font-size:1.5em;color:#6b7280">°C</span>
</div>
<div class="sensor-row">
<span>💧 Umidità:</span>
<span style="font-weight:600" id="humidity">--</span>
</div>
<div class="sensor-row">
<span>📶 WiFi Signal:</span>
<span style="font-weight:600" id="wifi-rssi">--</span>
</div>
</div>

<!-- Controllo Temperatura -->
<div class="card">
<h3>🎯 Temperatura Desiderata</h3>
<div class="temp-control">
<button class="temp-btn" onclick="decreaseTemp()">−</button>
<div class="temp-target" id="target-temp">21.0°C</div>
<button class="temp-btn" onclick="increaseTemp()">+</button>
</div>

<h3 style="margin-top:30px">⚙️ Modalità Operativa</h3>
<div class="mode-selector">
<button class="mode-btn" id="mode-comfort" onclick="setMode('comfort')">
☀️<br>Comfort
</button>
<button class="mode-btn" id="mode-eco" onclick="setMode('eco')">
🌙<br>Eco
</button>
<button class="mode-btn" id="mode-manual" onclick="setMode('manual')">
🎮<br>Manuale
</button>
</div>
</div>

<!-- Stato Sistema -->
<div class="card">
<h3>🎛️ Stato Sistema</h3>
<div style="display:flex;justify-content:space-between;align-items:center;margin:20px 0">
<span style="font-weight:600">Sistema Termostato:</span>
<div class="toggle-switch active" id="system-toggle" onclick="toggleSystem()">
<div class="toggle-knob"></div>
</div>
</div>
<div class="info-grid">
<div class="info-box">
<div class="info-label">🔥 Caldaia</div>
<div class="info-value" id="heating-state">OFF</div>
</div>
<div class="info-box">
<div class="info-label">⏱️ Tempo ON</div>
<div class="info-value" id="heating-time">0 min</div>
</div>
<div class="info-box">
<div class="info-label">🌡️ Setpoint</div>
<div class="info-value">
<span id="comfort-temp">21</span>°C / <span id="eco-temp">18</span>°C
</div>
</div>
</div>
</div>

<!-- Azioni -->
<div class="card">
<h3>⚡ Azioni</h3>
<button class="btn btn-primary" onclick="location.href='/update'">
⬆️ OTA Firmware Update
</button>
<button class="btn" onclick="restart()">
🔄 Riavvia Sistema
</button>
</div>

</div>

<script>
function updateData(){
fetch('/api/status').then(r=>r.json()).then(data=>{
const t=(data.current_temp!=null&&!isNaN(data.current_temp))?data.current_temp:20.0;
const h=(data.humidity!=null&&!isNaN(data.humidity))?data.humidity:50.0;
const tgt=(data.target_temp!=null)?data.target_temp:21.0;
const rssi=(data.wifi_rssi!=null)?data.wifi_rssi:-50;

document.getElementById('current-temp').textContent=t.toFixed(1);
document.getElementById('humidity').textContent=h.toFixed(1)+' %';
document.getElementById('target-temp').textContent=tgt.toFixed(1)+'°C';
document.getElementById('wifi-rssi').textContent=rssi+' dBm';

if(data.ip)document.getElementById('ip-info').textContent='IP: '+data.ip;

const badge=document.getElementById('heating-badge');
const state=document.getElementById('heating-state');
if(data.heating_on){
badge.textContent='🔥 Riscaldamento';
badge.className='badge badge-heating';
state.textContent='ON';
state.style.color='#dc2626';
}else{
badge.textContent='❄️ Idle';
badge.className='badge badge-idle';
state.textContent='OFF';
state.style.color='#2563eb';
}

const minutes=Math.floor((data.heating_time||0)/60);
document.getElementById('heating-time').textContent=minutes+' min';

const toggle=document.getElementById('system-toggle');
if(data.system_enabled)toggle.classList.add('active');
else toggle.classList.remove('active');

document.querySelectorAll('.mode-btn').forEach(btn=>btn.classList.remove('active'));
if(data.mode){
const modeBtn=document.getElementById('mode-'+data.mode);
if(modeBtn)modeBtn.classList.add('active');
}

if(data.comfort_temp)document.getElementById('comfort-temp').textContent=data.comfort_temp.toFixed(1);
if(data.eco_temp)document.getElementById('eco-temp').textContent=data.eco_temp.toFixed(1);

}).catch(err=>console.error('Error:',err));
}

function increaseTemp(){
fetch('/api/temp/increase',{method:'POST'}).then(()=>updateData()).catch(err=>console.error(err));
}

function decreaseTemp(){
fetch('/api/temp/decrease',{method:'POST'}).then(()=>updateData()).catch(err=>console.error(err));
}

function setMode(mode){
fetch('/api/mode?value='+mode,{method:'POST'}).then(()=>updateData()).catch(err=>console.error(err));
}

function toggleSystem(){
fetch('/api/system/toggle',{method:'POST'}).then(()=>updateData()).catch(err=>console.error(err));
}

function restart(){
if(confirm('Riavviare il termostato?\n\nDopo il riavvio dovrai ricaricare la pagina.')){
fetch('/api/restart',{method:'POST'});
setTimeout(()=>{
alert('Riavvio in corso...\nAttendi 10 secondi e ricarica la pagina.');
},500);
}
}

setInterval(updateData,2000);
updateData();
</script>
</body>
</html>
)rawliteral";

// ========================================
// OTA UPDATE PAGE
// ========================================
const char update_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>OTA Update - Wemos</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;padding:20px;display:flex;align-items:center;justify-content:center}
.container{background:white;border-radius:20px;padding:40px;max-width:500px;width:100%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}
h1{color:#667eea;margin-bottom:20px}
.info{background:#e0e7ff;border-left:4px solid #667eea;padding:15px;margin:20px 0;border-radius:8px}
.info p{color:#3730a3;font-size:0.9em;margin:5px 0}
.file-input{margin:30px 0;padding:20px;border:2px dashed #667eea;border-radius:12px;text-align:center;cursor:pointer;transition:all 0.3s}
.file-input:hover{border-color:#764ba2;background:#f9fafb}
.file-input input{display:none}
.btn{width:100%;padding:14px;border:none;border-radius:12px;font-size:1em;font-weight:600;cursor:pointer;background:linear-gradient(135deg,#667eea,#764ba2);color:white;margin-top:12px}
.btn:hover{opacity:0.9}
.btn:disabled{opacity:0.5;cursor:not-allowed}
.progress{margin-top:20px;height:30px;background:#e5e7eb;border-radius:15px;overflow:hidden;display:none}
.progress-bar{height:100%;background:linear-gradient(90deg,#667eea,#764ba2);transition:width 0.3s;display:flex;align-items:center;justify-content:center;color:white;font-weight:600}
.status{margin-top:20px;text-align:center;color:#6b7280}
.btn-back{background:#6b7280;margin-top:20px}
</style>
</head>
<body>
<div class="container">
<h1>🔄 OTA Firmware Update</h1>
<p style="color:#6b7280;margin-bottom:20px">Aggiorna il firmware Wemos</p>

<div class="info">
<p><strong>📝 Istruzioni:</strong></p>
<p>1. Compila: <code>pio run</code></p>
<p>2. File: <code>.pio/build/d1_mini/firmware.bin</code></p>
<p>3. Seleziona e carica qui sotto</p>
</div>

<form id="upload-form">
<div class="file-input" onclick="document.getElementById('file').click()">
<p id="file-name">📁 Clicca per selezionare firmware.bin</p>
<input type="file" id="file" accept=".bin" onchange="fileSelected()">
</div>
<button type="submit" class="btn" id="upload-btn" disabled>⬆️ Upload Firmware</button>
</form>

<div class="progress" id="progress">
<div class="progress-bar" id="progress-bar">0%</div>
</div>

<div class="status" id="status"></div>

<button class="btn btn-back" onclick="location.href='/'">← Torna alla Dashboard</button>
</div>

<script>
let selectedFile=null;

function fileSelected(){
selectedFile=document.getElementById('file').files[0];
if(selectedFile){
const sizeMB=(selectedFile.size/1024/1024).toFixed(2);
document.getElementById('file-name').textContent='✅ '+selectedFile.name+' ('+sizeMB+' MB)';
document.getElementById('upload-btn').disabled=false;
}
}

document.getElementById('upload-form').onsubmit=async function(e){
e.preventDefault();
if(!selectedFile){alert('Seleziona un file!');return;}

document.getElementById('upload-btn').disabled=true;
document.getElementById('progress').style.display='block';
document.getElementById('status').textContent='Upload in corso...';

const formData=new FormData();
formData.append('file',selectedFile);

try{
const xhr=new XMLHttpRequest();

xhr.upload.addEventListener('progress',(e)=>{
if(e.lengthComputable){
const percent=Math.round((e.loaded/e.total)*100);
document.getElementById('progress-bar').style.width=percent+'%';
document.getElementById('progress-bar').textContent=percent+'%';
}
});

xhr.addEventListener('load',()=>{
if(xhr.status===200){
document.getElementById('status').innerHTML='✅ Upload completato!<br>Wemos si sta riavviando...<br>Attendi 10 secondi...';
document.getElementById('status').style.color='#10b981';
setTimeout(()=>{location.href='/';},10000);
}else{
document.getElementById('status').textContent='❌ Errore: '+xhr.statusText;
document.getElementById('status').style.color='#ef4444';
document.getElementById('upload-btn').disabled=false;
}
});

xhr.addEventListener('error',()=>{
document.getElementById('status').textContent='❌ Errore di connessione';
document.getElementById('status').style.color='#ef4444';
document.getElementById('upload-btn').disabled=false;
});

xhr.open('POST','/update');
xhr.send(formData);

}catch(err){
document.getElementById('status').textContent='❌ Errore: '+err.message;
document.getElementById('status').style.color='#ef4444';
document.getElementById('upload-btn').disabled=false;
}
};
</script>
</body>
</html>
)rawliteral";

// ========================================
// LEGGI AHT10/AHT20
// ========================================
void readAHT() {
    if (!ahtFound) return;
    
    if (millis() - lastAHTRead >= AHT_READ_INTERVAL) {
        sensors_event_t humidity, temp;
        aht.getEvent(&humidity, &temp);
        
        if (!isnan(temp.temperature) && !isnan(humidity.relative_humidity)) {
            currentTemp = temp.temperature;
            currentHumidity = humidity.relative_humidity;
        }
        
        lastAHTRead = millis();
    }
}

// ========================================
// LOGICA RISCALDAMENTO
// ========================================
void updateHeating() {
    if (!systemEnabled) {
        if (heatingOn) {
            digitalWrite(RELAY_PIN, LOW);
            heatingOn = false;
        }
        return;
    }
    
    // Aggiorna target
    if (currentMode == MODE_COMFORT) targetTemp = comfortTemp;
    else if (currentMode == MODE_ECO) targetTemp = ecoTemp;
    
    // Logica isteresi
    if (!heatingOn) {
        if (currentTemp < (targetTemp - hysteresis)) {
            digitalWrite(RELAY_PIN, HIGH);
            heatingOn = true;
            lastHeatingChange = millis();
        }
    } else {
        if (currentTemp > (targetTemp + hysteresis)) {
            digitalWrite(RELAY_PIN, LOW);
            heatingOn = false;
            heatingOnTime += (millis() - lastHeatingChange) / 1000;
        }
    }
}

// ========================================
// MQTT DISCOVERY
// ========================================
void publishMQTTDiscovery() {
    if (mqttConfigSent) return;
    
    JsonDocument doc;
    String topic, payload;
    
    auto addDevice = [&]() {
        JsonObject device = doc["device"].to<JsonObject>();
        device["identifiers"][0] = device_id;
        device["name"] = device_name;
        device["model"] = "Wemos D1 Mini";
        device["manufacturer"] = "DIY";
    };
    
    // Temperatura sensor
    doc.clear();
    doc["name"] = "Temperatura Ambiente";
    doc["unique_id"] = String(device_id) + "_temp";
    doc["state_topic"] = String(mqtt_base_topic) + "/sensor/" + device_id + "/temperature/state";
    doc["unit_of_measurement"] = "°C";
    doc["device_class"] = "temperature";
    doc["value_template"] = "{{ value_json.temperature }}";
    addDevice();
    serializeJson(doc, payload);
    topic = String(mqtt_base_topic) + "/sensor/" + device_id + "/temperature/config";
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
    
    // Umidità sensor
    doc.clear();
    doc["name"] = "Umidità";
    doc["unique_id"] = String(device_id) + "_humidity";
    doc["state_topic"] = String(mqtt_base_topic) + "/sensor/" + device_id + "/humidity/state";
    doc["unit_of_measurement"] = "%";
    doc["device_class"] = "humidity";
    doc["value_template"] = "{{ value_json.humidity }}";
    addDevice();
    serializeJson(doc, payload);
    topic = String(mqtt_base_topic) + "/sensor/" + device_id + "/humidity/config";
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
    
    // Climate
    doc.clear();
    doc["name"] = "Termostato";
    doc["unique_id"] = String(device_id) + "_climate";
    doc["mode_state_topic"] = String(mqtt_base_topic) + "/climate/" + device_id + "/mode/state";
    doc["mode_command_topic"] = String(mqtt_base_topic) + "/climate/" + device_id + "/mode/set";
    doc["temperature_state_topic"] = String(mqtt_base_topic) + "/climate/" + device_id + "/temperature/state";
    doc["temperature_command_topic"] = String(mqtt_base_topic) + "/climate/" + device_id + "/temperature/set";
    doc["current_temperature_topic"] = String(mqtt_base_topic) + "/climate/" + device_id + "/current_temperature/state";
    doc["modes"][0] = "off";
    doc["modes"][1] = "heat";
    doc["min_temp"] = 15;
    doc["max_temp"] = 30;
    doc["temp_step"] = 0.5;
    addDevice();
    serializeJson(doc, payload);
    topic = String(mqtt_base_topic) + "/climate/" + device_id + "/config";
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
    
    mqttConfigSent = true;
}

// ========================================
// MQTT PUBLISH STATE
// ========================================
void publishMQTTState() {
    if (!mqttClient.connected()) return;
    
    JsonDocument doc;
    String topic, payload;
    
    // Temperatura
    doc.clear();
    doc["temperature"] = currentTemp;
    serializeJson(doc, payload);
    topic = String(mqtt_base_topic) + "/sensor/" + device_id + "/temperature/state";
    mqttClient.publish(topic.c_str(), payload.c_str());
    
    // Umidità
    doc.clear();
    doc["humidity"] = currentHumidity;
    serializeJson(doc, payload);
    topic = String(mqtt_base_topic) + "/sensor/" + device_id + "/humidity/state";
    mqttClient.publish(topic.c_str(), payload.c_str());
    
    // Climate mode
    topic = String(mqtt_base_topic) + "/climate/" + device_id + "/mode/state";
    mqttClient.publish(topic.c_str(), systemEnabled ? "heat" : "off");
    
    // Temperature setpoint
    topic = String(mqtt_base_topic) + "/climate/" + device_id + "/temperature/state";
    mqttClient.publish(topic.c_str(), String(targetTemp, 1).c_str());
    
    // Current temperature
    topic = String(mqtt_base_topic) + "/climate/" + device_id + "/current_temperature/state";
    mqttClient.publish(topic.c_str(), String(currentTemp, 1).c_str());
}

// ========================================
// MQTT CALLBACK
// ========================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    String modeCommandTopic = String(mqtt_base_topic) + "/climate/" + device_id + "/mode/set";
    if (String(topic) == modeCommandTopic) {
        systemEnabled = (message == "heat");
        publishMQTTState();
    }
    
    String tempCommandTopic = String(mqtt_base_topic) + "/climate/" + device_id + "/temperature/set";
    if (String(topic) == tempCommandTopic) {
        targetTemp = message.toFloat();
        if (targetTemp < 15.0) targetTemp = 15.0;
        if (targetTemp > 30.0) targetTemp = 30.0;
        currentMode = MODE_MANUAL;
        publishMQTTState();
    }
}

// ========================================
// MQTT RECONNECT
// ========================================
void reconnectMQTT() {
    if (mqttClient.connected()) return;
    
    if (mqttClient.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
        String modeCommandTopic = String(mqtt_base_topic) + "/climate/" + device_id + "/mode/set";
        String tempCommandTopic = String(mqtt_base_topic) + "/climate/" + device_id + "/temperature/set";
        mqttClient.subscribe(modeCommandTopic.c_str());
        mqttClient.subscribe(tempCommandTopic.c_str());
        
        publishMQTTDiscovery();
        publishMQTTState();
    }
}

// ========================================
// SETUP
// ========================================
void setup() {
    // Serial DISABILITATO per usare D9/D10 come I2C
    delay(500);
    
    // GPIO
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    
    // Software I2C per AHT10
    ahtWire.begin(AHT_SDA, AHT_SCL);
    
    // AHT10/AHT20
    if (aht.begin(&ahtWire)) {
        ahtFound = true;
        // Prima lettura
        delay(100);
        readAHT();
    } else {
        ahtFound = false;
    }
    
    // WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        attempts++;
    }
    
    // MQTT
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(1024);
    
    // OTA
    ArduinoOTA.setHostname("wemos-termostato");
    ArduinoOTA.setPassword("admin");
    ArduinoOTA.begin();
    
    // Web Server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });
    
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", update_html);
    });
    
    server.on("/update", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            bool success = !Update.hasError();
            AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", success ? "OK" : "FAIL");
            response->addHeader("Connection", "close");
            request->send(response);
            delay(1000);
            ESP.restart();
        },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!index) {
                Update.runAsync(true);
                if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
                    Update.printError(Serial);
                }
            }
            if (!Update.hasError()) {
                Update.write(data, len);
            }
            if (final) {
                if (!Update.end(true)) {
                    Update.printError(Serial);
                }
            }
        }
    );
    
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["current_temp"] = currentTemp;
        doc["humidity"] = currentHumidity;
        doc["target_temp"] = targetTemp;
        doc["heating_on"] = heatingOn;
        doc["system_enabled"] = systemEnabled;
        doc["heating_time"] = heatingOnTime;
        doc["wifi_rssi"] = WiFi.RSSI();
        doc["ip"] = WiFi.localIP().toString();
        doc["comfort_temp"] = comfortTemp;
        doc["eco_temp"] = ecoTemp;
        
        if (currentMode == MODE_COMFORT) doc["mode"] = "comfort";
        else if (currentMode == MODE_ECO) doc["mode"] = "eco";
        else doc["mode"] = "manual";
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.on("/api/temp/increase", HTTP_POST, [](AsyncWebServerRequest *request){
        targetTemp += 0.5;
        if (targetTemp > 30.0) targetTemp = 30.0;
        currentMode = MODE_MANUAL;
        request->send(200, "text/plain", "OK");
    });
    
    server.on("/api/temp/decrease", HTTP_POST, [](AsyncWebServerRequest *request){
        targetTemp -= 0.5;
        if (targetTemp < 15.0) targetTemp = 15.0;
        currentMode = MODE_MANUAL;
        request->send(200, "text/plain", "OK");
    });
    
    server.on("/api/mode", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("value")) {
            String mode = request->getParam("value")->value();
            if (mode == "comfort") currentMode = MODE_COMFORT;
            else if (mode == "eco") currentMode = MODE_ECO;
            else if (mode == "manual") currentMode = MODE_MANUAL;
        }
        request->send(200, "text/plain", "OK");
    });
    
    server.on("/api/system/toggle", HTTP_POST, [](AsyncWebServerRequest *request){
        systemEnabled = !systemEnabled;
        request->send(200, "text/plain", systemEnabled ? "ON" : "OFF");
    });
    
    server.on("/api/restart", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Restarting...");
        delay(1000);
        ESP.restart();
    });
    
    server.begin();
}

// ========================================
// LOOP
// ========================================
void loop() {
    ArduinoOTA.handle();
    
    if (!mqttClient.connected()) {
        static unsigned long lastReconnect = 0;
        if (millis() - lastReconnect > 5000) {
            reconnectMQTT();
            lastReconnect = millis();
        }
    }
    mqttClient.loop();
    
    readAHT();
    updateHeating();
    
    if (millis() - lastMQTTPublish >= MQTT_PUBLISH_INTERVAL) {
        publishMQTTState();
        lastMQTTPublish = millis();
    }
    
    delay(10);
}