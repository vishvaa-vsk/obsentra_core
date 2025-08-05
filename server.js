import express from 'express';
import cors from 'cors';
import os from 'os';
import { WebSocketServer } from 'ws';
import http from 'http';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
app.use(express.json());
app.use(cors());

const PORT = process.env.PORT || 3000;

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

// GET /status — Health check endpoint
app.get("/status", (req, res) => {
  res.json({ 
    status: "running",
    sensor: selectedSensor,
    clients: clients.size,
    timestamp: new Date().toISOString()
  });
});

// GET / — Root endpoint
app.get("/", (req, res) => {
  res.json({ 
    message: "Obsentra Core Server",
    version: "1.0.0",
    sensor: selectedSensor,
    websocket_clients: clients.size
  });
});

// GET /test — Serve WebSocket test page
app.get("/test", (req, res) => {
  res.sendFile(__dirname + '/websocket-test.html');
});

// POST /sensor-data — Receive sensor readings from ESP32
app.post("/sensor-data", (req, res) => {
  const { sensor, value } = req.body;
  console.log(`Data -> Sensor: ${sensor}, Value: ${value}`);

  // Broadcast to all WebSocket clients
  broadcast({
    type: "SENSOR_DATA",
    sensor,
    value,
    timestamp: Date.now()
  });

  res.json({ message: "Data received" });
});

// Create HTTP Server
const server = http.createServer(app);

// WebSocket Server
const wss = new WebSocketServer({ 
  server,
  path: '/' // Accept connections on root path
});

console.log('WebSocket Server initialized on path: /');

wss.on("connection", (ws, req) => {
  const clientIP = req.socket.remoteAddress;
  console.log(`✅ WebSocket Client Connected from ${clientIP}`);
  console.log(`   Connection headers:`, req.headers);
  clients.add(ws);

  // Send current sensor status immediately
  ws.send(JSON.stringify({
    type: "SENSOR_UPDATE",
    sensor: selectedSensor
  }));

  ws.on("message", (message) => {
    try {
      const data = JSON.parse(message.toString());
      console.log(`WebSocket Message:`, data);
      
      // Handle sensor update from WebSocket
      if (data.type === "SENSOR_UPDATE") {
        selectedSensor = data.sensor;
        console.log(`Sensor updated via WebSocket to: ${selectedSensor}`);
        
        // Broadcast to all other clients
        broadcast({
          type: "SENSOR_UPDATE",
          sensor: selectedSensor
        });
      }
    } catch (error) {
      console.log(`WebSocket Raw Message: ${message}`);
    }
  });

  ws.on("close", () => {
    console.log("WebSocket Client Disconnected");
    clients.delete(ws);
  });

  ws.on("error", (error) => {
    console.log("WebSocket Error:", error);
    clients.delete(ws);
  });
});

// Start Server
server.listen(PORT, '0.0.0.0', () => {
  const localIP = getLocalIP();
  console.log('=================================');
  console.log('🚀 Obsentra Server Started');
  console.log('=================================');
  console.log(`HTTP Server: http://localhost:${PORT}`);
  console.log(`HTTP Server: http://127.0.0.1:${PORT}`);
  console.log(`HTTP Server: http://${localIP}:${PORT}`);
  console.log(`WebSocket: ws://localhost:${PORT}`);
  console.log(`WebSocket: ws://127.0.0.1:${PORT}`);
  console.log(`WebSocket: ws://${localIP}:${PORT}`);
  console.log(`Current Sensor: ${selectedSensor}`);
  console.log('=================================');
  console.log('💡 Troubleshooting Tips:');
  console.log('- Try http://localhost:3000 in Windows browser');
  console.log('- If WSL mirrored networking, try the local IP');
  console.log('- Check Windows Firewall settings');
  console.log('=================================');
});

// Handle server errors
server.on('error', (error) => {
  console.error('Server Error:', error);
  if (error.code === 'EADDRINUSE') {
    console.log(`Port ${PORT} is already in use. Try a different port.`);
  }
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n=================================');
  console.log('🛑 Shutting down Obsentra Server');
  console.log('=================================');
  server.close(() => {
    process.exit(0);
  });
});

// Broadcast helper function
function broadcast(data) {
  const payload = JSON.stringify(data);
  let activeClients = 0;
  
  clients.forEach((client) => {
    if (client.readyState === 1) { // WebSocket.OPEN
      try {
        client.send(payload);
        activeClients++;
      } catch (error) {
        console.log('Error broadcasting to client:', error);
        clients.delete(client);
      }
    } else {
      // Remove dead connections
      clients.delete(client);
    }
  });
  
  if (activeClients > 0) {
    console.log(`Broadcasted to ${activeClients} clients: ${payload}`);
  }
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