//write GND firmware here
#include <esp_now.h>
#include <WiFi.h>
#include "config.h"

// Statistics
unsigned long packetsReceived = 0;
unsigned long packetsSent = 0;
unsigned long lastStatsTime = 0;

void onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
  packetsReceived++;
  
  // Forward packet to serial (to laptop)
  Serial.write(data, len);
  Serial.write('\n');  // Delimiter for Python parser
  
  // Optional: Print debug info
  /*
  Serial.print("Received from: ");
  for(int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if(i < 5) Serial.print(":");
  }
  Serial.printf(" Len: %d\n", len);
  */
}

void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  if(status == ESP_NOW_SEND_SUCCESS) {
    packetsSent++;
  } else {
    Serial.println("ERROR: Send failed");
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  
  Serial.println("\n=== Drone Mesh GCS ===");
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  // Print MAC address
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("GCS MAC Address: ");
  for(int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if(i < 5) Serial.print(":");
  }
  Serial.println();
  Serial.println("Copy this address to drone_simulator/config.h");
  Serial.println();
  
  // Initialize ESP-NOW
  if(esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW init failed");
    return;
  }
  
  // Register callbacks
  esp_now_register_recv_cb(onDataReceived);
  esp_now_register_send_cb(onDataSent);
  
  Serial.println("GCS Ready - Waiting for drones...");
  Serial.println("Listening on all channels");
  Serial.println();
}

void loop() {
  // Check for commands from serial (from laptop Python app)
  if(Serial.available()) {
    uint8_t commandBuffer[MAX_PACKET_SIZE];
    int len = 0;
    
    // Read until newline or max size
    while(Serial.available() && len < MAX_PACKET_SIZE) {
      uint8_t b = Serial.read();
      if(b == '\n') break;
      commandBuffer[len++] = b;
    }
    
    if(len > 0) {
      handleCommand(commandBuffer, len);
    }
  }
  
  // Print statistics every 5 seconds
  if(millis() - lastStatsTime > 5000) {
    Serial.printf("STATS: RX=%lu TX=%lu\n", packetsReceived, packetsSent);
    lastStatsTime = millis();
  }
  
  delay(10);
}

void handleCommand(uint8_t* cmd, int len) {
  // Validate packet format
  if(len < 4) {
    Serial.println("ERROR: Command too short");
    return;
  }
  
  if(cmd[0] != PACKET_START_BYTE) {
    Serial.println("ERROR: Invalid start byte");
    return;
  }
  
  uint8_t destDroneID = cmd[1];
  uint8_t packetType = cmd[3];
  
  Serial.printf("Sending command to Drone %d (Type: %d)\n", destDroneID, packetType);
  
  // Broadcast to all drones (they filter by ID)
  uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_err_t result = esp_now_send(broadcastAddr, cmd, len);
  
  if(result != ESP_OK) {
    Serial.printf("ERROR: Send failed (code %d)\n", result);
  }
}