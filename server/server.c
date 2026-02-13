#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// for windows D:
// #include <Winsock2.h>
// #include <ws2tcpip.h>

// for Unix
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9000
#define BUFFER_SIZE 1024

void receive_data(int socket_fd);

int main() {
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0); // create datagram socket
    if (sock < 0) {
        perror("Opening socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // bind socket address to port
    if (bind(sock, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("UDP server listening on port %d\n", PORT);

    while(1) {
        receive_data(sock);
    }

    close(sock);
    return 0;
}

void receive_data(int socket_fd) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    memset(buffer, 0, BUFFER_SIZE);
    memset(&client_addr, 0, sizeof(client_addr));

    ssize_t recv_len = recvfrom(socket_fd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_addr, &addr_len);
    if (recv_len == -1) {
        perror("recvfrom failed");
        return;
    }

    buffer[recv_len] = '\0';

    printf("Received packet from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    printf("Data: %s\n", buffer);
}
