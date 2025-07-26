/*
 * Obsentra ESP32 Firmware
 * Intelligent Sensor Management System
 * 
 * Core Features:
 * - Multi-sensor support (DHT22, Flame, MQ-2)
 * - WebSocket communication with backend
 * - Wi-Fi management with SoftAP fallback
 * - Persistent configuration storage
 * - Dynamic sensor selection via HTTP API
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoWebsockets.h>
#include <EEPROM.h>
#include <DHT.h>
#include <ArduinoJson.h>

// Pin Definitions
#define DHT_PIN 4
#define FLAME_PIN 5
#define MQ2_PIN 35

// Sensor Types
#define DHT_TYPE DHT22

// EEPROM Configuration
#define EEPROM_SIZE 512
#define WIFI_SSID_ADDR 0
#define WIFI_PASS_ADDR 64
#define SERVER_URL_ADDR 128
#define SENSOR_TYPE_ADDR 192

// Network Configuration
const char* AP_SSID = "Obsentra-Setup";
const char* AP_PASS = "obsentra123";

// Global Variables
DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);
using namespace websockets;
WebsocketsClient wsClient;

String wifiSSID = "";
String wifiPassword = "";
String serverURL = "";
String selectedSensor = "NONE";

bool isConnectedToWiFi = false;
bool isConnectedToWS = false;
bool isAPMode = false;
unsigned long lastSensorRead = 0;
unsigned long lastReconnectAttempt = 0;

const unsigned long SENSOR_INTERVAL = 2000;  // Read sensors every 2 seconds
const unsigned long RECONNECT_INTERVAL = 30000; // Try to reconnect every 30 seconds

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize
  
  Serial.println();
  Serial.println("================================");
  Serial.println("Obsentra ESP32 Firmware Starting");
  Serial.println("================================");
  
  // Initialize pins
  pinMode(FLAME_PIN, INPUT);
  Serial.println("Pins initialized");
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  Serial.println("EEPROM initialized");
  
  // Check if this is first boot (check for magic number)
  if (EEPROM.read(EEPROM_SIZE - 1) != 0xAA) {
    Serial.println("First boot detected - clearing EEPROM");
    clearEEPROM();
    EEPROM.write(EEPROM_SIZE - 1, 0xAA); // Set magic number
    EEPROM.commit();
  }
  
  // Initialize DHT sensor
  dht.begin();
  Serial.println("DHT sensor initialized");
  
  // Load configuration from EEPROM
  loadConfiguration();
  
  // Try to connect to Wi-Fi
  if (wifiSSID.length() > 0) {
    connectToWiFi();
  }
  
  // If Wi-Fi connection fails, start AP mode
  if (!isConnectedToWiFi) {
    startAPMode();
  }
  
  // Initialize web server routes
  setupWebServer();
  
  Serial.println("================================");
  Serial.println("Obsentra ESP32 Ready!");
  Serial.println("================================");
}

void loop() {
  // Handle web server
  server.handleClient();
  
  // If in normal mode (connected to Wi-Fi)
  if (isConnectedToWiFi && !isAPMode) {
    // Check WebSocket connection
    if (!isConnectedToWS) {
      if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
        connectToWebSocket();
        lastReconnectAttempt = millis();
      }
    } else {
      wsClient.poll();
    }
    
    // Read and send sensor data
    if (millis() - lastSensorRead > SENSOR_INTERVAL) {
      readAndSendSensorData();
      lastSensorRead = millis();
    }
  }
  
  delay(100);
}

void loadConfiguration() {
  Serial.println("Loading configuration from EEPROM...");
  
  wifiSSID = readStringFromEEPROM(WIFI_SSID_ADDR, 32);
  wifiPassword = readStringFromEEPROM(WIFI_PASS_ADDR, 32);
  serverURL = readStringFromEEPROM(SERVER_URL_ADDR, 64);
  selectedSensor = readStringFromEEPROM(SENSOR_TYPE_ADDR, 16);
  
  // Clean up any invalid data
  wifiSSID.trim();
  wifiPassword.trim();
  serverURL.trim();
  selectedSensor.trim();
  
  if (selectedSensor.length() == 0) {
    selectedSensor = "NONE";
  }
  
  Serial.println("Configuration loaded:");
  Serial.println("SSID: " + wifiSSID);
  Serial.println("Server: " + serverURL);
  Serial.println("Sensor: " + selectedSensor);
}

void saveConfiguration() {
  Serial.println("Saving configuration to EEPROM...");
  
  writeStringToEEPROM(WIFI_SSID_ADDR, wifiSSID);
  writeStringToEEPROM(WIFI_PASS_ADDR, wifiPassword);
  writeStringToEEPROM(SERVER_URL_ADDR, serverURL);
  writeStringToEEPROM(SENSOR_TYPE_ADDR, selectedSensor);
  
  EEPROM.commit();
  Serial.println("Configuration saved!");
}

void clearEEPROM() {
  for (int i = 0; i < EEPROM_SIZE - 1; i++) {
    EEPROM.write(i, 0);
  }
}

String readStringFromEEPROM(int addr, int maxLen) {
  String result = "";
  for (int i = 0; i < maxLen; i++) {
    char c = EEPROM.read(addr + i);
    if (c == 0) break;
    // Only add printable ASCII characters
    if (c >= 32 && c <= 126) {
      result += c;
    } else {
      break; // Stop if we hit non-printable character
    }
  }
  return result;
}

void writeStringToEEPROM(int addr, String data) {
  // Clear the entire section first
  for (int i = 0; i < 32; i++) {
    EEPROM.write(addr + i, 0);
  }
  
  // Write the actual data
  for (int i = 0; i < data.length() && i < 31; i++) {
    EEPROM.write(addr + i, data.charAt(i));
  }
  EEPROM.write(addr + data.length(), 0); // Null terminator
}

void connectToWiFi() {
  Serial.println("Connecting to Wi-Fi: " + wifiSSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    isConnectedToWiFi = true;
    Serial.println();
    Serial.println("Wi-Fi Connected!");
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    Serial.println();
    Serial.println("Wi-Fi Connection Failed");
    isConnectedToWiFi = false;
  }
}

void startAPMode() {
  Serial.println("Starting Access Point Mode...");
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  
  isAPMode = true;
  isConnectedToWiFi = false;
  
  Serial.println("AP Mode Started");
  Serial.println("SSID: " + String(AP_SSID));
  Serial.println("Password: " + String(AP_PASS));
  Serial.println("IP: " + WiFi.softAPIP().toString());
}

void connectToWebSocket() {
  if (serverURL.length() == 0) {
    Serial.println("No server URL configured");
    return;
  }
  
  String wsURL = "ws://" + serverURL + ":3000";
  Serial.println("Connecting to WebSocket: " + wsURL);
  
  wsClient.onMessage([](WebsocketsMessage message) {
    Serial.println("WebSocket message: " + message.data());
    handleWebSocketMessage(message.data());
  });
  
  wsClient.onEvent([](WebsocketsEvent event, String data) {
    if (event == WebsocketsEvent::ConnectionOpened) {
      Serial.println("WebSocket Connected!");
      isConnectedToWS = true;
    } else if (event == WebsocketsEvent::ConnectionClosed) {
      Serial.println("WebSocket Disconnected");
      isConnectedToWS = false;
    }
  });
  
  if (wsClient.connect(wsURL)) {
    isConnectedToWS = true;
  } else {
    Serial.println("WebSocket connection failed");
    isConnectedToWS = false;
  }
}

void handleWebSocketMessage(String message) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, message);
  
  String type = doc["type"].as<String>();
  
  if (type == "SENSOR_UPDATE") {
    selectedSensor = doc["sensor"].as<String>();
    Serial.println("Sensor updated to: " + selectedSensor);
    saveConfiguration();
  }
}

void readAndSendSensorData() {
  if (selectedSensor == "NONE" || !isConnectedToWS) {
    return;
  }
  
  String sensorValue = "";
  
  if (selectedSensor == "DHT22") {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    if (!isnan(temperature) && !isnan(humidity)) {
      sensorValue = String(temperature) + "°C, " + String(humidity) + "%";
      Serial.println("DHT22 -> " + sensorValue);
    } else {
      Serial.println("DHT22 read error");
      return;
    }
  }
  else if (selectedSensor == "FLAME") {
    int flameValue = digitalRead(FLAME_PIN);
    sensorValue = (flameValue == LOW) ? "FLAME_DETECTED" : "NO_FLAME";
    Serial.println("Flame -> " + sensorValue);
  }
  else if (selectedSensor == "MQ2") {
    int gasValue = analogRead(MQ2_PIN);
    sensorValue = String(gasValue);
    Serial.println("MQ2 -> " + sensorValue);
  }
  
  // Send data via WebSocket
  if (sensorValue.length() > 0) {
    DynamicJsonDocument doc(1024);
    doc["sensor"] = selectedSensor;
    doc["value"] = sensorValue;
    doc["timestamp"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    wsClient.send(jsonString);
  }
}

void setupWebServer() {
  // Serve configuration page
  server.on("/", HTTP_GET, []() {
    String html = generateConfigPage();
    server.send(200, "text/html", html);
  });
  
  // Handle configuration form
  server.on("/config", HTTP_POST, []() {
    wifiSSID = server.arg("ssid");
    wifiPassword = server.arg("password");
    serverURL = server.arg("server");
    
    saveConfiguration();
    
    server.send(200, "text/html", 
      "<h1>Configuration Saved!</h1>"
      "<p>Device will restart and connect to your network.</p>"
      "<script>setTimeout(function(){ window.location.href='/'; }, 3000);</script>"
    );
    
    delay(1000);
    ESP.restart();
  });
  
  // API endpoint to get current sensor
  server.on("/api/sensor", HTTP_GET, []() {
    server.send(200, "application/json", "{\"sensor\":\"" + selectedSensor + "\"}");
  });
  
  // API endpoint to set sensor
  server.on("/api/sensor", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, server.arg("plain"));
      selectedSensor = doc["sensor"].as<String>();
      saveConfiguration();
      server.send(200, "application/json", "{\"status\":\"success\",\"sensor\":\"" + selectedSensor + "\"}");
    } else {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request\"}");
    }
  });
  
  server.begin();
  Serial.println("Web server started");
}

String generateConfigPage() {
  String html = "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>Obsentra Configuration</title>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<style>"
    "body { font-family: Arial; margin: 20px; background: #f0f0f0; }"
    ".container { max-width: 400px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }"
    "h1 { color: #333; text-align: center; }"
    "input { width: 100%; padding: 10px; margin: 5px 0; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }"
    "button { width: 100%; padding: 12px; background: #007bff; color: white; border: none; border-radius: 5px; cursor: pointer; }"
    ".status { margin: 10px 0; padding: 10px; border-radius: 5px; background: #d1ecf1; color: #0c5460; }"
    "</style>"
    "</head>"
    "<body>"
    "<div class=\"container\">"
    "<h1>Obsentra Setup</h1>"
    "<div class=\"status\">"
    "<strong>Current Status:</strong><br>"
    "Mode: " + String(isAPMode ? "Setup" : "Connected") + "<br>"
    "Sensor: " + selectedSensor + "<br>"
    "IP: " + String(isAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + 
    "</div>"
    "<form action=\"/config\" method=\"post\">"
    "<h3>Wi-Fi Configuration</h3>"
    "<input type=\"text\" name=\"ssid\" placeholder=\"Wi-Fi Network Name\" value=\"" + wifiSSID + "\" required>"
    "<input type=\"password\" name=\"password\" placeholder=\"Wi-Fi Password\" value=\"\">"
    "<h3>Server Configuration</h3>"
    "<input type=\"text\" name=\"server\" placeholder=\"Server IP Address\" value=\"" + serverURL + "\" required>"
    "<button type=\"submit\">Save & Connect</button>"
    "</form>"
    "</div>"
    "</body>"
    "</html>";
  return html;
}
