import express from 'express';
import cors from 'cors';
import os from 'os';
import { WebSocketServer } from 'ws';

const app = express();
app.use(express.json());
app.use(cors());

const PORT = 3000;

let selectedSensor = "NONE";
let clients = new Set(); // Store connected WebSocket clients

// POST /set-sensor — Set sensor selection
app.post("/set-sensor", (req, res) => {
selectedSensor = req.body.sensor;
console.log(`Sensor updated to: ${selectedSensor}`);

// Notify all WebSocket clients
broadcast({
type: "SENSOR_UPDATE",
sensor: selectedSensor
});

res.json({ message: "Sensor updated", sensor: selectedSensor });
});

// GET /get-sensor — Get current sensor selection
app.get("/get-sensor", (req, res) => {
res.json({ sensor: selectedSensor });
});

// POST /sensor-data — Receive sensor readings from ESP32
app.post("/sensor-data", (req, res) => {
const { sensor, value } = req.body;
console.log(`📡 Data → Sensor: ${sensor}, Value: ${value}`);

// Broadcast to all WebSocket clients
broadcast({
type: "SENSOR_DATA",
sensor,
value
});

res.json({ message: "Data received" });
});

// Start HTTP Server
const server = app.listen(PORT, '0.0.0.0', () => {
console.log(`✅ Obsentra server running at http://${getLocalIP()}:${PORT}`);
});

// WebSocket Server
const wss = new WebSocketServer({ server });

wss.on("connection", (ws) => {
console.log("🔗 WebSocket Client Connected");
clients.add(ws);

// Send current sensor immediately
ws.send(JSON.stringify({
type: "SENSOR_UPDATE",
sensor: selectedSensor
}));

ws.on("message", (message) => {
console.log(`💬 WebSocket → ${message}`);
});

ws.on("close", () => {
console.log("❌ WebSocket Client Disconnected");
clients.delete(ws);
});
});

// Broadcast helper function
function broadcast(data) {
const payload = JSON.stringify(data);
clients.forEach((client) => {
if (client.readyState === 1) {
client.send(payload);
}
});
}

// Get local IP address
function getLocalIP() {
const nets = os.networkInterfaces();
for (const name of Object.keys(nets)) {
for (const net of nets[name]) {
if (net.family === 'IPv4' && !net.internal) {
return net.address;
}
}
}
return 'localhost';
}