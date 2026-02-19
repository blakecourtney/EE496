// header file here
#ifndef CONFIG_H
#define CONFIG_H

// IMPORTANT: Update this with your GCS MAC address!
uint8_t gcsAddress[] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0x00};

// Drone configuration
#define DRONE_ID 1  // Change this for each simulator (1, 2, 3...)

// Starting position (change for each drone)
#define START_LAT 37.7749    // San Francisco
#define START_LON -122.4194
#define START_ALT 100.0      // meters

// Update rates
#define TELEMETRY_RATE_MS 100  // 10 Hz
#define HEARTBEAT_RATE_MS 1000  // 1 Hz

// Packet configuration (must match GCS)
#define MAX_PACKET_SIZE 250
#define PACKET_START_BYTE 0xFE
#define PACKET_END_BYTE 0xFF

// Packet types
#define PKT_TYPE_TELEMETRY 1
#define PKT_TYPE_COMMAND 2
#define PKT_TYPE_VIDEO 3
#define PKT_TYPE_HEARTBEAT 4

// Command types
#define CMD_ARM 10
#define CMD_DISARM 11
#define CMD_START_VIDEO 20
#define CMD_STOP_VIDEO 21

#endif
