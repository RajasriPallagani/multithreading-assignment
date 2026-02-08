#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <errno.h>
 
#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define THREAD_POOL_SIZE 5
#define SERIAL_DEVICE "/dev/ttyUSB0"
#define BAUD_RATE B9600
 
// --- Global Shared Resources ---
int serial_fd;
int client_sockets[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t serial_mutex = PTHREAD_MUTEX_INITIALIZER;
 
// --- Thread Pool Structure ---
typedef struct {
    void (*function)(void *);
    void *argument;
} thread_task_t;
 
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t threads[THREAD_POOL_SIZE];
    thread_task_t queue[256];
    int count;
    int head;
    int tail;
    int shutdown;
} threadpool_t;
 
threadpool_t pool;
 
// --- ROBUST SERIAL SETUP ---
int setup_serial_port(const char *portname) {
    int fd = open(portname, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }
 
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) return -1;
 
    cfsetospeed(&tty, BAUD_RATE);
    cfsetispeed(&tty, BAUD_RATE);
 
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= (CLOCAL | CREAD);
 
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;
 
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 0;
 
    if (tcsetattr(fd, TCSANOW, &tty) != 0) return -1;
    tcflush(fd, TCIOFLUSH);
    return fd;
}
 
// --- Helper: Broadcast to TCP ---
void broadcast_to_clients(char *msg) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] > 0) {
            send(client_sockets[i], msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}
 
// --- Helper: Send to UART ---
void send_to_uart(char *msg) {
    pthread_mutex_lock(&serial_mutex);
    write(serial_fd, msg, strlen(msg));
    pthread_mutex_unlock(&serial_mutex);
}
 
// --- TCP Client Handler ---
void handle_client(void *arg) {
    int sock = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    char uart_buffer[BUFFER_SIZE + 50];
    int n;
 
    printf("[Thread] Handling new client socket %d\n", sock);
 
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        
        if (n <= 0) {
            printf("[Thread] Client %d disconnected.\n", sock);
            close(sock);
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == sock) {
                    client_sockets[i] = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            return;
        }
 
        buffer[n] = '\0';
        printf("[TCP Recv]: %s\n", buffer);
 
        // Format for UART and send
        snprintf(uart_buffer, sizeof(uart_buffer), "TCP: %s\r\n", buffer);
        send_to_uart(uart_buffer);
    }
}
 
// --- Thread Pool Worker ---
void *threadpool_worker(void *arg) {
    while (1) {
        pthread_mutex_lock(&pool.lock);
        while (pool.count == 0 && !pool.shutdown) {
            pthread_cond_wait(&pool.notify, &pool.lock);
        }
        if (pool.shutdown) {
            pthread_mutex_unlock(&pool.lock);
            pthread_exit(NULL);
        }
        thread_task_t task = pool.queue[pool.head];
        pool.head = (pool.head + 1) % 256;
        pool.count--;
        pthread_mutex_unlock(&pool.lock);
        (*(task.function))(task.argument);
    }
}
 
void threadpool_init() {
    pool.count = 0; pool.head = 0; pool.tail = 0; pool.shutdown = 0;
    pthread_mutex_init(&pool.lock, NULL);
    pthread_cond_init(&pool.notify, NULL);
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
        pthread_create(&pool.threads[i], NULL, threadpool_worker, NULL);
}
 
void threadpool_add(void (*function)(void *), void *argument) {
    pthread_mutex_lock(&pool.lock);
    pool.queue[pool.tail].function = function;
    pool.queue[pool.tail].argument = argument;
    pool.tail = (pool.tail + 1) % 256;
    pool.count++;
    pthread_cond_signal(&pool.notify);
    pthread_mutex_unlock(&pool.lock);
}
 
// --- Serial Reader Thread ---
void *serial_reader_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    fd_set readfds;
 
    printf("[Serial Thread] Started monitoring %s...\n", SERIAL_DEVICE);
 
    while(1) {
        FD_ZERO(&readfds);
        FD_SET(serial_fd, &readfds);
 
        int activity = select(serial_fd + 1, &readfds, NULL, NULL, NULL);
 
        if (activity < 0 && errno != EINTR) {
            perror("Serial Select Error");
        }
        
        if (FD_ISSET(serial_fd, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            int n = read(serial_fd, buffer, BUFFER_SIZE - 1);
            if (n > 0) {
                buffer[n] = '\0';
                printf("[UART Recv]: %s\n", buffer);
                fflush(stdout);
                broadcast_to_clients(buffer);
            }
        }
    }
}
 
// --- Main ---
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    fd_set readfds;
    char admin_buffer[BUFFER_SIZE];
    char fmt_buffer[BUFFER_SIZE + 50];
 
    threadpool_init();
    
    serial_fd = setup_serial_port(SERIAL_DEVICE);
    if (serial_fd < 0) {
        printf("FAILED to open serial port. Check permissions (sudo?).\n");
        exit(EXIT_FAILURE);
    }
    printf("UART initialized on %s (Raw/VMIN=1)\n", SERIAL_DEVICE);
 
    pthread_t serial_thread;
    if(pthread_create(&serial_thread, NULL, serial_reader_thread, NULL) != 0) {
        perror("Failed to create serial thread");
        exit(1);
    }
 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed"); exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt"); exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
 
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed"); exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("Listen"); exit(EXIT_FAILURE);
    }
 
    printf("Server listening on TCP port %d\n", PORT);
    printf("Admin Console: You can type here to broadcast to EVERYONE.\n");
 
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);        // Watch TCP Server Socket
        FD_SET(STDIN_FILENO, &readfds);     // Watch Admin Keyboard (The missing piece!)
 
        int max_fd = (server_fd > STDIN_FILENO) ? server_fd : STDIN_FILENO;
 
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
 
        if ((activity < 0) && (errno != EINTR)) printf("Select error\n");
 
        // 1. Handle New TCP Connections
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                perror("Accept"); continue;
            }
            
            printf("New TCP Client connected! Socket: %d\n", new_socket);
 
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
 
            int *pclient = malloc(sizeof(int));
            *pclient = new_socket;
            threadpool_add(handle_client, pclient);
        }
 
        // 2. Handle Admin Keyboard Input (Server Console)
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(admin_buffer, 0, BUFFER_SIZE);
            if (read(STDIN_FILENO, admin_buffer, BUFFER_SIZE - 1) > 0) {
                // Remove trailing newline if present for cleaner log
                admin_buffer[strcspn(admin_buffer, "\n")] = 0;
 
                // Format: [Admin]: <Message>
                snprintf(fmt_buffer, sizeof(fmt_buffer), "Server Admin: %s\r\n", admin_buffer);
 
                // A. Send to TCP Clients
                broadcast_to_clients(fmt_buffer);
                
                // B. Send to UART (USB)
                send_to_uart(fmt_buffer);
 
                printf("[Sent]: %s", fmt_buffer); // Local feedback
            }
        }
    }
    return 0;
}
 
