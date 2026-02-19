#ifndef CONFIG_H
#define CONFIG_H

// Serial configuration
#define SERIAL_BAUD 115200

// Packet configuration
#define MAX_PACKET_SIZE 250
#define PACKET_START_BYTE 0xFE
#define PACKET_END_BYTE 0xFF

// Packet types
#define PKT_TYPE_TELEMETRY 1

#endif
