#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9000
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    char *message;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP address> <message>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    message = argv[2];

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Opening socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (sendto(sock, message, strlen(message), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("sendto");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Message sent: %s\n", message);

    close(sock);
    return 0;
}