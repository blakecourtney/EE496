#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    #define CLOSE_SOCKET closesocket
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #define CLOSE_SOCKET close
#endif

#define PORT 9000
#define BUFFER_SIZE 1024

void receive_data(int socket_fd);

int main() {

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
#endif

    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Opening socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        CLOSE_SOCKET(sock);
        return 1;
    }

    printf("UDP server listening on port %d\n", PORT);

    while (1) {
        receive_data(sock);
    }

    CLOSE_SOCKET(sock);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}

void receive_data(int socket_fd) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    memset(buffer, 0, BUFFER_SIZE);
    memset(&client_addr, 0, sizeof(client_addr));

    int recv_len = recvfrom(socket_fd, buffer, BUFFER_SIZE - 1, 0,
                            (struct sockaddr *)&client_addr, &addr_len);

    if (recv_len == -1) {
        perror("recvfrom failed");
        return;
    }

    buffer[recv_len] = '\0';

    printf("Received packet from %s:%d\n",
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));

    printf("Data: %s\n\n", buffer);
}
