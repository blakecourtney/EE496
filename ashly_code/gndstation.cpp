#include <iostream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>

// packet builder
std::vector<uint8_t> build_waypoint_packet(uint8_t drone_id,
                                           float lat,
                                           float lon,
                                           float alt) {
    std::vector<uint8_t> packet;
    packet.push_back(0xFE);
    packet.push_back(drone_id);
    packet.push_back(5); // PKT_TYPE_WAYPOINT

    uint8_t payload[64] = {0};
    std::memcpy(payload + 0, &lat, 4);
    std::memcpy(payload + 4, &lon, 4);
    std::memcpy(payload + 8, &alt, 4);

    packet.insert(packet.end(), payload, payload + 64);
    packet.push_back(0xFF);

    return packet;
}