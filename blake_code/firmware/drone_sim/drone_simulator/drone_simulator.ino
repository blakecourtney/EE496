//drop TLM output here for testing
#include <esp_now.h>
#include <WiFi.h>
#include "config.h"

// Simulated drone state
struct DroneState {
  float lat;
  float lon;
  float alt;
  float roll;
  float pitch;
  float yaw;
  float battery;
  uint8_t satellites;
  bool armed;
  bool streaming_video;
} drone;

unsigned long lastTelemetryTime = 0;
unsigned long lastHeartbeatTime = 0;
unsigned long flightStartTime = 0;

void onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
  // Check if packet is for this drone
  if(len < 4) return;
  if(data[0] != PACKET_START_BYTE) return;
  
  uint8_t destID = data[1];
  if(destID != DRONE_ID && destID != 0) return;  // 0 = broadcast
  
  uint8_t packetType = data[3];
  
  if(packetType == PKT_TYPE_COMMAND) {
    if(len < 5) return;
    uint8_t cmdType = data[4];
    handleCommand(cmdType);
  }
}

void handleCommand(uint8_t cmdType) {
  Serial.print("Received command: ");
  
  switch(cmdType) {
    case CMD_ARM:
      Serial.println("ARM");
      drone.armed = true;
      flightStartTime = millis();
      break;
      
    case CMD_DISARM:
      Serial.println("DISARM");
      drone.armed = false;
      break;
      
    case CMD_START_VIDEO:
      Serial.println("START VIDEO");
      drone.streaming_video = true;
      break;
      
    case CMD_STOP_VIDEO:
      Serial.println("STOP VIDEO");
      drone.streaming_video = false;
      break;
      
    default:
      Serial.printf("UNKNOWN (%d)\n", cmdType);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.printf("\n=== Drone Simulator %d ===\n", DRONE_ID);
  
  // Initialize drone state
  drone.lat = START_LAT;
  drone.lon = START_LON;
  drone.alt = START_ALT;
  drone.roll = 0;
  drone.pitch = 0;
  drone.yaw = 0;
  drone.battery = 12.6;
  drone.satellites = 12;
  drone.armed = false;
  drone.streaming_video = false;
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  // Print MAC address
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("Drone MAC Address: ");
  for(int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if(i < 5) Serial.print(":");
  }
  Serial.println();
  
  // Initialize ESP-NOW
  if(esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW init failed");
    return;
  }
  
  // Register GCS as peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, gcsAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if(esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ERROR: Failed to add GCS peer");
    return;
  }
  
  // Register receive callback
  esp_now_register_recv_cb(onDataReceived);
  
  Serial.println("Drone Simulator Ready");
  Serial.printf("Target GCS: %02X:%02X:%02X:%02X:%02X:%02X\n",
                gcsAddress[0], gcsAddress[1], gcsAddress[2],
                gcsAddress[3], gcsAddress[4], gcsAddress[5]);
  Serial.println();
}

void loop() {
  unsigned long now = millis();
  
  // Simulate drone dynamics
  simulateFlight();
  
  // Send telemetry
  if(now - lastTelemetryTime >= TELEMETRY_RATE_MS) {
    sendTelemetry();
    lastTelemetryTime = now;
  }
  
  // Send heartbeat
  if(now - lastHeartbeatTime >= HEARTBEAT_RATE_MS) {
    sendHeartbeat();
    lastHeartbeatTime = now;
  }
  
  delay(10);
}

void simulateFlight() {
  float dt = 0.1;  // 100ms in seconds
  
  if(drone.armed) {
    // Simulate movement
    float timeInFlight = (millis() - flightStartTime) / 1000.0;
    
    drone.lat += 0.00001 * cos(timeInFlight * 0.1);  // Circular pattern
    drone.lon += 0.00001 * sin(timeInFlight * 0.1);
    drone.alt += random(-10, 10) * 0.1;
    
    // Keep altitude in reasonable range
    if(drone.alt < 50) drone.alt = 50;
    if(drone.alt > 200) drone.alt = 200;
    
    // Simulate attitude oscillation
    drone.roll = sin(timeInFlight) * 15;
    drone.pitch = cos(timeInFlight * 0.8) * 10;
    drone.yaw += 0.5;
    if(drone.yaw > 360) drone.yaw -= 360;
    
    // Battery drain
    drone.battery -= 0.0001;
    if(drone.battery < 10.5) drone.battery = 10.5;  // Minimum
  } else {
    // On ground - stable
    drone.roll = 0;
    drone.pitch = 0;
  }
}

void sendTelemetry() {
  uint8_t packet[MAX_PACKET_SIZE];
  size_t len = 0;
  
  packet[len++] = PACKET_START_BYTE;
  packet[len++] = DRONE_ID;  // Source
  packet[len++] = 0;          // Destination (0 = GCS)
  packet[len++] = PKT_TYPE_TELEMETRY;
  
  // Pack telemetry data
  memcpy(&packet[len], &drone.lat, sizeof(float)); len += sizeof(float);
  memcpy(&packet[len], &drone.lon, sizeof(float)); len += sizeof(float);
  memcpy(&packet[len], &drone.alt, sizeof(float)); len += sizeof(float);
  memcpy(&packet[len], &drone.roll, sizeof(float)); len += sizeof(float);
  memcpy(&packet[len], &drone.pitch, sizeof(float)); len += sizeof(float);
  memcpy(&packet[len], &drone.yaw, sizeof(float)); len += sizeof(float);
  memcpy(&packet[len], &drone.battery, sizeof(float)); len += sizeof(float);
  packet[len++] = drone.satellites;
  packet[len++] = drone.armed ? 1 : 0;
  packet[len++] = drone.streaming_video ? 1 : 0;
  
  packet[len++] = PACKET_END_BYTE;
  
  esp_err_t result = esp_now_send(gcsAddress, packet, len);
  
  if(result != ESP_OK) {
    Serial.printf("Telemetry send failed: %d\n", result);
  }
}

void sendHeartbeat() {
  uint8_t packet[10];
  packet[0] = PACKET_START_BYTE;
  packet[1] = DRONE_ID;
  packet[2] = 0;  // GCS
  packet[3] = PKT_TYPE_HEARTBEAT;
  packet[4] = drone.armed ? 1 : 0;
  packet[5] = PACKET_END_BYTE;
  
  esp_now_send(gcsAddress, packet, 6);
  Serial.println("♥");  // Heartbeat indicator
}