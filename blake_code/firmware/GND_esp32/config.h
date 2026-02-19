//GND station config file
#ifndef CONFIG_H
#define CONFIG_H

// Serial configuration
#define SERIAL_BAUD 115200

// ESP-NOW configuration
#define ESPNOW_CHANNEL 1
#define MAX_PEERS 10

// Packet configuration
#define MAX_PACKET_SIZE 250
#define PACKET_START_BYTE 0xFE
#define PACKET_END_BYTE 0xFF

// Packet types
#define PKT_TYPE_TELEMETRY 1
#define PKT_TYPE_COMMAND 2
#define PKT_TYPE_VIDEO 3
#define PKT_TYPE_HEARTBEAT 4

#endif