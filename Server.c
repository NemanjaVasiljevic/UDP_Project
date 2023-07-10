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
    int window;
    int seqNum;
    int ackNum;
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

    int serverWindow = 0;
    int clientWindow = 0;
    int lastProcessedSeqNum = -1;

    printf("Enter server window size in bytes: ");
    scanf("%d", &serverWindow);
    getchar(); // Consume the newline character

    if (recvfrom(serverSocket, &clientWindow, sizeof(int), 0, (struct sockaddr*)&clientAddress, &clientAddressLength) < 0) {
        perror("Packet receiving failed");
        exit(EXIT_FAILURE);
    }

    printf("Window from client received (%d bytes). Sending my window size...\n", clientWindow);
    if (sendto(serverSocket, &serverWindow, sizeof(int), 0, (struct sockaddr*)&clientAddress, clientAddressLength) < 0) {
        perror("Packet sending failed");
        exit(EXIT_FAILURE);
    }

    printf("Handshake complete\nUDP Server is running and waiting for packets...\n");
    fflush(stdin);


    sleep(10);

    while (1) {
        if (recvfrom(serverSocket, buffer, sizeof(Packet), 0, (struct sockaddr*)&clientAddress, &clientAddressLength) < 0) {
            perror("Packet receiving failed");
            exit(EXIT_FAILURE);
        }

        Packet receivedPacket;
        memcpy(&receivedPacket, buffer, sizeof(Packet));

        if (receivedPacket.header.seqNum <= lastProcessedSeqNum && receivedPacket.header.seqNum != -1) {
            printf("Duplicate message received. Ignoring...\n");
            continue;
        }

        printf("Message: ");
        for (int i = 0; i <= receivedPacket.header.lastByteIndex; i++) {
            printf("%c", receivedPacket.message.context[i]);
        }
        printf("\n");

        lastProcessedSeqNum = receivedPacket.header.seqNum;

        int nextSeq = receivedPacket.header.seqNum;

        if (receivedPacket.header.seqNum != -1) {
            if ((strlen(receivedPacket.message.context) - 1) == receivedPacket.header.lastByteIndex) {
                printf("Successfully received %d bytes. Next sequence starts at index %d\n", receivedPacket.header.seqNum, receivedPacket.header.seqNum + 1);
            }
        } else {
            nextSeq = 0; // Indicates that the message is received in its entirety and there is no next sequence
        }

        if (sendto(serverSocket, &nextSeq, sizeof(nextSeq), 0, (struct sockaddr*)&clientAddress, clientAddressLength) < 0) {
            perror("Packet sending failed");
            exit(EXIT_FAILURE);
        }
    }

    // Close the socket
    close(serverSocket);

    return 0;
}
