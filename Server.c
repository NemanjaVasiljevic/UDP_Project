#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>

#define SERVER_PORT 8888
#define BUFFER_SIZE 1024

typedef struct{
    int messageNum;
    char message[BUFFER_SIZE];
}Data;


int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    // Set the socket to non-blocking mode
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        exit(1);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        exit(1);
    }

    while (1) {
        // Use select() for non-blocking receive
        char* buffer = (char*)malloc(sizeof(Data));
        Data* data = (Data*)malloc(sizeof(Data));

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 1;  // Set a timeout of 1 second
        timeout.tv_usec = 0;

        int select_result = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
        if (select_result == -1) {
            perror("select");
            exit(1);
        } else if (select_result == 0) {
            printf("No data received.\n");
        } else {
            if (FD_ISSET(sockfd, &read_fds)) {
                memset(buffer, 0, BUFFER_SIZE);
                socklen_t client_addr_len = sizeof(client_addr);

                // Receive data from a client
                if (recvfrom(sockfd, buffer, sizeof(Data), 0, (struct sockaddr *)&client_addr, &client_addr_len) == -1) {
                    perror("recvfrom");
                    exit(1);
                }
                
                memcpy(data,buffer,sizeof(Data));

                printf("Received from client: %d %s\n", data->messageNum, data->message);

                // Echo the received message back to the client
                if (sendto(sockfd, "ACK", 4, 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) {
                    perror("sendto");
                    exit(1);
                }
            }
        }

        free(buffer);
        free(data);
    }

    // Close the socket
    close(sockfd);

    return 0;
}
