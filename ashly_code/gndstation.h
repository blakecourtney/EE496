#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <vector>
#include <cstdint>

int open_serial(const char* port);
void send_packet(int fd, const std::vector<uint8_t>& packet);
void close_serial(int fd);

#endif