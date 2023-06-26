#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_MESSAGE_LEN 1024
#define SERVER_PORT 8888

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

int main() {
    int serverSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    char buffer[sizeof(Packet)];

    // Create UDP socket
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Socket binding failed");
        exit(EXIT_FAILURE);
    }

    printf("UDP Server is running and waiting for packets...\n");

    // Receive packet from client
    Packet receivedPacket;
    while(1){
        
        char* ack = (char*)malloc(sizeof(char));

        sleep(10);
        printf("Prosao 10s sleep\n");

        if (recvfrom(serverSocket, buffer, sizeof(Packet), 0, (struct sockaddr*)&clientAddress, &clientAddressLength) < 0) {
            perror("Packet receiving failed");
            exit(EXIT_FAILURE);
        }

        memcpy(&receivedPacket, buffer, sizeof(Packet));

        printf("Received packet from client:\n");
        printf("First Byte Index: %d\n", receivedPacket.header.firstByteIndex);
        printf("Last Byte Index: %d\n", receivedPacket.header.lastByteIndex);
        printf("Message: %s\n", receivedPacket.message.context);

        int i = 0;

        while(i<=receivedPacket.header.lastByteIndex){
            ack = &receivedPacket.message.context[i];

            printf("Postavio ack na %c index je %d\n",*ack, i);
            if (sendto(serverSocket, ack, sizeof(char), 0, (struct sockaddr*)&clientAddress, clientAddressLength) < 0) {
                perror("Packet sending failed");
                exit(EXIT_FAILURE);
            }
            if (recvfrom(serverSocket, &i, sizeof(int), 0, (struct sockaddr*)&clientAddress, &clientAddressLength) < 0) {
                perror("Packet receiving failed");
                exit(EXIT_FAILURE);
            }
            if(i == -1){
                break;
            }
        }
        ack=NULL;
        free(ack);
    }
    // Close the socket
    close(serverSocket);

    return 0;
}
