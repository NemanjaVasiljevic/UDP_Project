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
#define MAX_MESSAGE_LEN 20

int SendMessageLength(int* length, int sockfd, struct sockaddr_in server_addr);


typedef struct{
    int messageLength;
    char message[MAX_MESSAGE_LEN];
}Data;


static int messageCounter = 0;


int main() {

#pragma region SOCKET_SETUP     
    
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

#pragma endregion
    // Send data to the server

    while(1){

        Data* data = (Data*)malloc(sizeof(Data));


        printf("Enter message: ");
        scanf("%s",data->message);
        char byte;
        char* buffer = (char*)malloc(sizeof(Data));
        memcpy(buffer, data, sizeof(Data));

        size_t messageLen = strlen(data->message);
        size_t bytesSent = 0;
        
        if(messageLen > MAX_MESSAGE_LEN){

            printf("The message length was exceeded please write something shorter\n");

        }else{


            data->messageLength = messageLen;                      
            if(SendMessageLength((int *)&messageLen,sockfd,server_addr)!=1){

                printf("The message length is too great for the server to accept\n");

            }else{
                printf("Upao u while za slanje\n");


                while(bytesSent<messageLen){
                    printf("Upao u while za slanje bajtova\n");

                    byte = data->message[bytesSent];

                    if (sendto(sockfd, &byte, sizeof(char), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
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
                        /*Implementirati ponovno slanje*/

                    } else {
                        if (FD_ISSET(sockfd, &read_fds)) {
                            char ackByte;
                            socklen_t server_addr_len = sizeof(server_addr);

                            if (recvfrom(sockfd, &ackByte, sizeof(char), 0, (struct sockaddr *)&server_addr, &server_addr_len) == -1) {
                                perror("recvfrom");
                                exit(1);
                            }

                            //Ispis ACK prijema
                            printf("Received: %c\n", ackByte);
                            if(ackByte == byte){
                                bytesSent++;
                            }
                        }

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



int SendMessageLength(int* length, int sockfd, struct sockaddr_in server_addr){

    int ackByte;


    if (sendto(sockfd, length, sizeof(int), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
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
        return 0;

    } else if (select_result == 0) {

        printf("Timeout reached. Resending message...\n");
        return 0;

    } else {
        if (FD_ISSET(sockfd, &read_fds)) {
            socklen_t server_addr_len = sizeof(server_addr);

            if (recvfrom(sockfd, &ackByte, sizeof(int), 0, (struct sockaddr *)&server_addr, &server_addr_len) == -1) {
                perror("recvfrom");
                return 0;
            }

            //Ispis ACK prijema
            printf("Received: %d\n", ackByte);
        }
        
    }

    if(ackByte!=0){
        return 1;
    }else{
        return 0;
    }
}
