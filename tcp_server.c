#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
 
#define PORT 8080
#define MAX_CLIENTS 5
 
void* client(void* arg)
{
    int client_fd = *(int*)arg;
    char buffer[256];
    int n;
 
    while ((n = read(client_fd, buffer, sizeof(buffer)-1)) > 0)
    {
        buffer[n] = '\0';
        printf("Client %d: %s\n", client_fd, buffer);
 
        printf("You: ");
        fgets(buffer, sizeof(buffer), stdin);
        write(client_fd, buffer, strlen(buffer)+1);
    }
 
    printf("Client %d disconnected\n", client_fd);
    close(client_fd);
    pthread_exit(NULL);
}
 
int main()
{
    int server_fd, client_fd;
    struct sockaddr_in addr;
    pthread_t threads[MAX_CLIENTS];
    socklen_t addr_len = sizeof(addr);
 
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
 
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
 
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, MAX_CLIENTS);
 
    printf("Server listening on port %d...\n", PORT);
 
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_fd = accept(server_fd, NULL, NULL);
        printf("New client connected: %d\n", client_fd);
        pthread_create(&threads[i], NULL, client, &client_fd);
    }
 
    for (int i = 0; i < MAX_CLIENTS; i++)
        pthread_join(threads[i], NULL);
 
    close(server_fd);
    return 0;
}
