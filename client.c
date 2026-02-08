#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024
 
int main(int argc, char const *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <SERVER_IP_ADDRESS>\n", argv[0]);
        return -1;
    }
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    fd_set readfds;
 
    // Create Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
 
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
 
    printf("Connected to Server! Start typing to chat via UART bridge...\n");
 
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // Watch Standard Input (Keyboard)
        FD_SET(sock, &readfds);         // Watch Server Socket
 
        int max_sd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;
 
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
 
        if (activity < 0) {
            printf("Select error\n");
            break;
        }
 
        // If User Typed something
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            if (read(STDIN_FILENO, buffer, BUFFER_SIZE) > 0) {
                send(sock, buffer, strlen(buffer), 0);
            }
        }
 
        // If Server sent data (from UART)
        if (FD_ISSET(sock, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            int valread = read(sock, buffer, BUFFER_SIZE);
            if (valread == 0) {
                printf("Server disconnected.\n");
                break;
            }
            printf("%s", buffer); // Print message from server/UART
        }
    }
 
    close(sock);
    return 0;
}
