#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_MESSAGE_LEN 24
#define SERVER_PORT 8888
#define TIMEOUT_SECONDS 3


typedef struct {
    char context[MAX_MESSAGE_LEN];
} Message;

typedef struct {
    int firstByteIndex;
    int lastByteIndex;
} Header;

typedef struct {
    Header header;
    Message message;
} Packet;


int ResendMessage(Packet* packet, int clientSocket, struct sockaddr_in serverAddress);



int main() {
    int clientSocket;
    struct sockaddr_in serverAddress;

    // Create UDP socket
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Change to the server's IP address

    // Prepare packet
    while(1){

        int resendCounter = 3;

        Packet* packet = (Packet*)malloc(sizeof(Packet));
        Packet* backupPacket = (Packet*)malloc(sizeof(Packet));
        char* ack = (char*)malloc(sizeof(char));
        

        packet->header.firstByteIndex = 0;
        printf("Enter your message:");
        fgets(packet->message.context,MAX_MESSAGE_LEN,stdin);
        packet->header.lastByteIndex = strlen(packet->message.context)-2;


        // Send packet to the server
        if (sendto(clientSocket, packet, sizeof(Packet), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            perror("Packet sending failed");
            exit(EXIT_FAILURE);
        }

        printf("Packet sent to the server.\n");

        // Set up timer
        struct timeval timeout;
        timeout.tv_sec = TIMEOUT_SECONDS;
        timeout.tv_usec = 0;

        // Set up file descriptor set for select()
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);

        // Wait for response or timeout
        int selectResult = select(clientSocket + 1, &readfds, NULL, NULL, &timeout);
        int resendRes = 0;
        if (selectResult == -1) {
            perror("Select error");
            exit(EXIT_FAILURE);


        }else if(selectResult == 0) {
            // Timeout occurred, resend the message
            while(resendCounter!=0 || resendRes != 1){
                printf("Timeout occurred, attempts left %d. Resending message...\n",resendCounter);
                resendRes = ResendMessage(backupPacket,clientSocket,serverAddress);
                resendCounter--;
            }
            resendCounter = 3;
        }



        backupPacket = packet;
        socklen_t serverAddressLength = sizeof(serverAddress);

        for(int i = packet->header.firstByteIndex; i<=packet->header.lastByteIndex;i++){
            //proveri byte po byte da li je to to
            int tempAck;
            if (recvfrom(clientSocket, ack, sizeof(char), 0, (struct sockaddr*)&serverAddress, &serverAddressLength) < 0) {
                perror("Packet receiving failed");
                exit(EXIT_FAILURE);
            }
            printf("Poredim sad %c sa servera sa %c sa klijenta\n",*ack,packet->message.context[i]);
            if(strcmp(ack, &packet->message.context[i])==0){

                printf("Byte at index %d is not the same as the original one (Server:%c Client:%c)",i,*ack,packet->message.context[i]);
                printf("\nResending message.....\n");
                tempAck = -1;
                if (sendto(clientSocket, &tempAck, sizeof(int), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
                    perror("Packet sending failed");
                    exit(EXIT_FAILURE);
                }

                // Set up timer
                struct timeval timeout;
                timeout.tv_sec = TIMEOUT_SECONDS;
                timeout.tv_usec = 0;

                // Set up file descriptor set for select()
                fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(clientSocket, &readfds);

                // Wait for response or timeout
                int selectResult = select(clientSocket + 1, &readfds, NULL, NULL, &timeout);
                int resendRes = 0;
                if (selectResult == -1) {
                    perror("Select error");
                    exit(EXIT_FAILURE);


                }else if(selectResult == 0) {
                    // Timeout occurred, resend the message
                    while(resendCounter!=0 || resendRes != 1){
                        printf("Timeout occurred, attempts left %d. Resending message...\n",resendCounter);
                        resendRes = ResendMessage(backupPacket,clientSocket,serverAddress);
                        resendCounter--;
                    }
                    resendCounter = 3;
                }


                ResendMessage(packet,clientSocket,serverAddress);
                
            }else{
                printf("Byte at server at index %d is %c and original is %c\n",i,*ack,packet->message.context[i]);
                tempAck = i+1;
                if (sendto(clientSocket, &tempAck, sizeof(int), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
                    perror("Packet sending failed");
                    exit(EXIT_FAILURE);
                }
            }
            
        }
        ack = NULL;
        free(ack);
        free(packet);
    }
    // Close the socket
    close(clientSocket);

    return 0;
}

int ResendMessage(Packet* packet, int clientSocket, struct sockaddr_in serverAddress){
    if (sendto(clientSocket, packet, sizeof(Packet), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            perror("Packet sending failed");
            exit(EXIT_FAILURE);
            return 0;
        }

        printf("Packet sent to the server.\n");

        // Set up timer
        struct timeval timeout;
        timeout.tv_sec = TIMEOUT_SECONDS;
        timeout.tv_usec = 0;

        // Set up file descriptor set for select()
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);

        // Wait for response or timeout
        int selectResult = select(clientSocket + 1, &readfds, NULL, NULL, &timeout);

        if (selectResult == -1) {
            perror("Select error");
            exit(EXIT_FAILURE);


        }else if(selectResult == 0) {
            return 0;
        }else{
            return 1;
        }
}
