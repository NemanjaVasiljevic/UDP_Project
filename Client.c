#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <time.h>

#define MAX_MESSAGE_LEN 1024
#define SERVER_PORT 8888
#define TIMEOUT_SECONDS 3
#define MAX_RESEND_ATTEMPTS 2

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

#pragma region socketSetup 
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


    //Inicijalno slanje poruke "handshake" sa serverom gde se razmenjuju velicine prozora
    int clientWindow = 0;
    int serverWindow = 0;

    printf("Enter client window size in bytes: ");
    scanf("%d",&clientWindow);
    getchar(); // Consume the newline character

    if (sendto(clientSocket, &clientWindow, sizeof(int), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            perror("Packet sending failed");
            exit(EXIT_FAILURE);
        }

    printf("Window (%d bytes) sent to the server. Waiting for response...\n",clientWindow);

    socklen_t serverAddressLength = sizeof(serverAddress);
    if (recvfrom(clientSocket, &serverWindow, sizeof(char), 0, (struct sockaddr*)&serverAddress, &serverAddressLength) < 0) {
                perror("Packet receiving failed");
                exit(EXIT_FAILURE);
    }
    printf("Server window (%d bytes) recived from the server. Handshake complete.\n",serverWindow);


#pragma endregion

    // Prepare packet
    char partedMessage[MAX_MESSAGE_LEN];

    while(1){

        Packet* packet = (Packet*)malloc(sizeof(Packet));
        char* messageBuffer = (char*)malloc(MAX_MESSAGE_LEN*sizeof(char));
        

        packet->header.firstByteIndex = 0;
        packet->header.window = serverWindow;

        printf("Enter your message:");
        fgets(messageBuffer,MAX_MESSAGE_LEN,stdin);
        messageBuffer[strcspn(messageBuffer, "\n")] = '\0'; //brise enter prilikom unosa

        packet->header.lastByteIndex = strlen(messageBuffer)-1;
        packet->header.ackNum = -1; //za slucaj da moze celo da se posalje
        packet->header.seqNum = -1; //za slucaj da moze celo da se posalje



        //delim poruke na parcice ukoliko je to potrebno
        if(strlen(messageBuffer)-1 > serverWindow){

            double x  = (double)packet->header.lastByteIndex/(double)serverWindow;
            double messageParts = ceil(x);
            int j = 0;
            int byteCount = 0;
            

            while(messageParts != 0){

                j = 0;
                memset(partedMessage, 0, sizeof(partedMessage)); 
                
                for (int i = packet->header.ackNum+1; i <= packet->header.ackNum + serverWindow; i++)
                {   
                    if(messageBuffer[i]!='\0'){
                        partedMessage[j] = messageBuffer[i];
                        j++;
                    }else{
                        break;
                    }
                }

                byteCount++;
                packet->header.seqNum = byteCount; //ovo predstavlja redni broj poslednjeg bajta u nizu koji se salje
                memcpy(packet->message.context,partedMessage,sizeof(partedMessage));
                packet->header.firstByteIndex = 0;
                packet->header.lastByteIndex = j - 1; //ovde se azurira ukupna duzina poruke

                int resendCounter = 0;
                int serverResponse = 0;

                while (resendCounter <= MAX_RESEND_ATTEMPTS) {
                    if (sendto(clientSocket, packet, sizeof(Packet), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
                        perror("Packet sending failed");
                        exit(EXIT_FAILURE);
                    }

                    printf("\n\nPacket was sent to the server.\n");

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
                    } else if (selectResult == 0) {

                        // Timeout occurred, resend the message
                        resendCounter++;
                        printf("Timeout occurred, attempts left %d. Resending message...\n", MAX_RESEND_ATTEMPTS - resendCounter + 1);
                    } else {

                        // Response received
                        socklen_t serverAddressLength = sizeof(serverAddress);
                        if (recvfrom(clientSocket, &serverResponse, sizeof(serverResponse), 0, (struct sockaddr*)&serverAddress, &serverAddressLength) < 0) {
                            perror("Packet receiving failed");
                            exit(EXIT_FAILURE);
                        }

                        printf("Server responded with ack %d \n", serverResponse);

                        if (serverResponse == packet->header.seqNum) {
                            // Server acknowledged the message, exit the resend loop
                            break;
                        } else {
                            // Server response received, but not the expected ack, resend the message
                            resendCounter++;
                            printf("Invalid ack received, attempts left %d. Resending message...\n", MAX_RESEND_ATTEMPTS - resendCounter + 1);
                        }
                    }
                }

                if (resendCounter > MAX_RESEND_ATTEMPTS) {
                    printf("Maximum resend attempts reached. Aborting...\n");
                    break;
                }

                packet->header.ackNum = (serverResponse * serverWindow)-1; // (ukupno koliko je primio)
                printf("\nHEADER ACKNUM %d\n",packet->header.ackNum);
                messageParts--;
            }

            printf("\nPoslata poruka %s\n",messageBuffer);

        }else{
            // Salje se cela poruka
            strcpy(packet->message.context,messageBuffer);
            
            int resendCounter = 0;
            int serverResponse = 0;

            while (resendCounter <= MAX_RESEND_ATTEMPTS) {
                if (sendto(clientSocket, packet, sizeof(Packet), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
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

                if (selectResult == -1) {
                    perror("Select error");
                    exit(EXIT_FAILURE);
                } else if (selectResult == 0) {
                    // Timeout occurred, resend the message
                    resendCounter++;
                    printf("Timeout occurred, attempts left %d. Resending message...\n", MAX_RESEND_ATTEMPTS - resendCounter + 1);
                } else {
                    // Response received
                    socklen_t serverAddressLength = sizeof(serverAddress);
                    if (recvfrom(clientSocket, &serverResponse, sizeof(serverResponse), 0, (struct sockaddr*)&serverAddress, &serverAddressLength) < 0) {
                        perror("Packet receiving failed");
                        exit(EXIT_FAILURE);
                    }

                    printf("Server responded with ack %d\n", serverResponse);

                    if (serverResponse == 0) {
                        // Server acknowledged the message, exit the resend loop
                        break;
                    } else {
                        // Server response received, but not the expected ack, resend the message
                        resendCounter++;
                        printf("Invalid ack received, attempts left %d. Resending message...\n", MAX_RESEND_ATTEMPTS - resendCounter + 1);
                    }
                }
            }

            if (resendCounter > MAX_RESEND_ATTEMPTS) {
                printf("Maximum resend attempts reached. Aborting...\n");
            }
        }

        free(packet);
        free(messageBuffer);
    }
    // Close the socket
    close(clientSocket);

    return 0;
}
