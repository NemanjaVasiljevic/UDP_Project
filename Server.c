#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>

#define SERVER_PORT 8888
#define BUFFER_SIZE 1024
#define MAX_MESSAGE_LEN 20

typedef struct{
    int messageNum;
    char message[MAX_MESSAGE_LEN];
}Data;

int CheckMessageLength(int sockfd, int* length, struct sockaddr_in client_addr);
int checked = 1;
int byteCounter=0;

int main() {

#pragma region SOCKET_SETUP

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

#pragma endregion

    while (1) {

        char byte;
        int messageLength;

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
            checked = 1;

        } else {
            if (FD_ISSET(sockfd, &read_fds)) {
                
                socklen_t client_addr_len = sizeof(client_addr);
                int length=0;
                if(checked == 1){
                    messageLength = CheckMessageLength(sockfd,&length,client_addr);
                }
                if(messageLength!=0){
                    checked = 0;
                    //printf("Dobio poruku koju mogu da primim sad radim primanje bajta po bajt\n");
                    sleep(1);

                    // Receive data from a client
                    if (recvfrom(sockfd, &byte, sizeof(char), 0, (struct sockaddr *)&client_addr, &client_addr_len) == -1) {
                        perror("recvfrom");
                        exit(1);
                    }
                    
                    if(byteCounter < messageLength-1){
                        printf("Received Byte: %c\n", byte);
                        data->message[byteCounter] = byte;
                        byteCounter++;
                    }else{
                        printf("Finished sending full message is: %s\n",data->message);
                        byteCounter=0;
                    }

                    // Echo the received message back to the client
                    if (sendto(sockfd, &byte, sizeof(char), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) {
                        perror("sendto");
                        exit(1);
                    }
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


int CheckMessageLength(int sockfd, int* length, struct sockaddr_in client_addr){
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = 1;  // Set a timeout of 1 second
    timeout.tv_usec = 0;

    int select_result = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

    if (select_result == -1) {
        perror("select");
        return 0;

    } else if (select_result == 0) {
        printf("No data received.\n");
        return 0;

    } else {
        socklen_t client_addr_len = sizeof(client_addr);

        if (recvfrom(sockfd, length, sizeof(int), 0, (struct sockaddr *)&client_addr, &client_addr_len) == -1) {
            perror("recvfrom");
            return 0;
        }

        printf("Received length: %d\n", *length);

        if(*length < MAX_MESSAGE_LEN){

            // Echo the received message back to the client
            if (sendto(sockfd, length, sizeof(int), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) {
                perror("sendto");
                return 0;
            }
            return *length;
        }else{

            // Echo the received message back to the client
            if (sendto(sockfd, 0, sizeof(int), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) {
                perror("sendto");
                return 0;
            }
            return 0;
        }
    }
}