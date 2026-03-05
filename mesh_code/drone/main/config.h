#ifndef CONFIG_H
#define CONFIG_H

// Mesh Network
#define MESH_ID             {0x77, 0x77, 0x77, 0x77, 0x77, 0x77} // can set this
#define MESH_CHANNEL        1
#define MESH_MAX_LAYER      6  // number of hops allowed

// Serial: GND station to/from GUI
#define SERIAL_BAUD         115200

// Packet Types
#define PKT_TYPE_TELEMETRY  1  // GPS, attitude, battery
#define PKT_TYPE_COMMAND    2  // arm, disarm, etc.
#define PKT_TYPE_FLAG       3  // ML person detection alert
#define PKT_TYPE_HEARTBEAT  4  // keep-alive
#define PKT_TYPE_WAYPOINT   5  // search coordinates
#define PKT_TYPE_PHOTO      6  // photo chunk
#define PKT_TYPE_ACK        7  // acknowledge flag or photo transfer

// Packet Framing
#define PKT_START           0xFE
#define PKT_END             0xFF

// Telemetry Packet Payload
typedef struct {
    float lat;
    float lon;
    float alt;        // meters (AGL)
    float roll;
    float pitch;
    float yaw;
    float battery;
    uint8_t satellites;
    uint8_t armed;
} telemetry_t;

// Flag Packet Payload
typedef struct {
    float lat;         
    float lon;
    float alt;         // meters 
    float confidence;  // ML confidence score 0.0-1.0
} flag_t;

// Waypoint Packet Payload
typedef struct {
    float lat;
    float lon;
    float alt;         // meters
} waypoint_t;

// Photo Packet
#define PHOTO_CHUNK_SIZE  230 //set this after testing
typedef struct {
    uint8_t  start;             // PKT_START 
    uint8_t  drone_id;          
    uint8_t  type;              // PKT_TYPE_PHOTO
    uint16_t chunk_index;       
    uint16_t total_chunks;      
    uint16_t data_len;
    uint8_t  data[PHOTO_CHUNK_SIZE];
    uint8_t  end;               // PKT_END (0xFF)
} photo_packet_t;

// ACK Payload (GND -> drone)
typedef struct {
    uint8_t  ack_type;      // which packet type is being ACKed (PKT_TYPE_FLAG or PKT_TYPE_PHOTO)
    uint16_t chunk_index;   // for PKT_TYPE_PHOTO: index of chunk ACKed; for PKT_TYPE_FLAG: 0
} ack_t;

// Generic Packet Wrapper for all packets EXCEPT PHOTO DATA
typedef struct {
    uint8_t start;      // PKT_START (0xFE)
    uint8_t drone_id;   // which drone
    uint8_t type;       // PKT_TYPE_  - set to one of the above
    uint8_t payload[64];
    uint8_t end;        // PKT_END (0xFF)
} packet_t;

#endif
