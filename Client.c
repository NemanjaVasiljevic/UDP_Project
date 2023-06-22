#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define BUFFER_SIZE 1024


typedef struct{
    int messageNum;
    char message[BUFFER_SIZE];
}Data;


static int messageCounter = 0;


int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

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

    // Send data to the server

    while(1){

        Data* data = (Data*)malloc(sizeof(Data));

        data->messageNum = ++messageCounter;
        printf("Enter message: ");
        scanf("%s",data->message);
        //printf("Uneta poruka %s", data->message);
        char* buffer = (char*)malloc(sizeof(Data));
        memcpy(buffer, data, sizeof(Data));

        if (sendto(sockfd, buffer, sizeof(Data), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("sendto");
            exit(1);
        }

        // Use select() for non-blocking receive
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 5;  // Set a timeout of 5 seconds
        timeout.tv_usec = 0;

        int select_result = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
        if (select_result == -1) {
            perror("select");
            exit(1);
        } else if (select_result == 0) {

            printf("Timeout reached. Resending message...\n");
            

        } else {
            if (FD_ISSET(sockfd, &read_fds)) {
                // Receive response from the server
                memset(buffer, 0, BUFFER_SIZE);
                socklen_t server_addr_len = sizeof(server_addr);
                if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &server_addr_len) == -1) {
                    perror("recvfrom");
                    exit(1);
                }

                // Print the echoed message from the server
                printf("Received: %s\n", buffer);
            }

            free(buffer);
            free(data);
        }
    }
    // Close the socket
    close(sockfd);

    return 0;
}
