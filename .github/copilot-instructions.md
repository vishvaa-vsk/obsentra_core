# 🧠 Project Overview: Obsentra
  
  Obsentra is an intelligent and scalable sensor management system designed for industrial, agricultural, and research use cases. It enables wireless, modular sensor deployment via an ESP32-based IoT edge device, a Node.js-based backend, and a Flutter-based mobile application for configuration, monitoring, and control.

# 🔧 System Architecture

1.Edge Device (ESP32 with firmware)

 - Acts as a central hardware hub to which various sensors can be connected.

 - Supports dynamic sensor selection (e.g., DHT22, Flame Sensor, MQ-2).

 - Uses WebSocket protocol to send real-time sensor data to the backend.

 - Has a built-in SoftAP fallback mode for Wi-Fi setup via mobile or browser.

 - Stores Wi-Fi credentials persistently using EEPROM.

2. Backend Server (obsentra_core - Node.js)

 - Serves HTTP endpoints for setting/getting the selected sensor.

 - Hosts a WebSocket server to receive data from ESP32 and broadcast to clients.

 - Intended to be extended for:

   - Database storage of sensor logs

   - User authentication

   - Multi-device management

3. Mobile App (obsentra_shield - Flutter)

 - User interface for selecting the active sensor (e.g., switch between DHT22, Flame).

 - Displays real-time sensor readings via WebSocket connection.

 - Future support for:

   - Wi-Fi configuration via SoftAP connection

   - Device pairing & provisioning

   - Graphs, alerts, and threshold-based notifications

# 🧩 Core Features

- ✅ Plug-and-play support for multiple sensors
- ✅ Dynamic sensor selection from the app
- ✅ WebSocket-based real-time data streaming
- ✅ Persistent Wi-Fi configuration with SoftAP fallback
- ✅ Modular backend for easy extension and analytics
- ✅ Mobile-first management experience via Flutter app

# 🚀 Target Use Cases

- Industrial environmental monitoring (e.g., gas, temperature, fire)

- Smart agriculture (soil, humidity, climate sensors)

- Enterprise-grade IoT deployments with custom sensor networks

- Maker and startup prototyping of modular sensor hubs

# 📈 Planned Extensions

- Cloud integration for remote monitoring

- Alert system for abnormal readings

- OTA firmware updates

- Local data buffering when offline

- Role-based access control and analytics dashboards