#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
 
#define PORT 8080
 
int main()
{
    int sock;
    struct sockaddr_in server;
    char msg[256];
    char buffer[256];
    int n;
 
    sock = socket(AF_INET, SOCK_STREAM, 0);
 
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
 
    if(connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        printf("Connection failed\n");
        return 1;
    }
 
    printf("Connected to server. Type messages:\n");
 
    while (1)
    {
        printf("You: ");
        fgets(msg, sizeof(msg), stdin);
        write(sock, msg, strlen(msg)+1);
 
        n = read(sock, buffer, sizeof(buffer)-1);
        if(n <= 0)
        {
            printf("Server disconnected\n");
            break;
        }
        buffer[n] = '\0';
        printf("Server: %s\n", buffer);
    }
 
    close(sock);
    return 0;
}
