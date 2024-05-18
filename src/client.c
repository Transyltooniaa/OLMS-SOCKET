#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include "../header/client.h"
#include "../header/clientMenu.h"


// Thread synchronization variables
pthread_mutex_t mutex;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Global variables
char buffer[BUFFER_SIZE] = {0};
int sock = 0;
volatile sig_atomic_t timeout_flag = 0; // Timeout flag




// Function to execute loginMenu with mutex protection
void loginMenuWrapper() {
    pthread_mutex_lock(&mutex);
    loginMenu(sock);
    pthread_mutex_unlock(&mutex);
}


void connectToServer(const char *server_ip) {
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }

    read(sock, buffer, BUFFER_SIZE);
    printf("%s\n", buffer);
    memset(buffer, 0, BUFFER_SIZE);

    loginMenuWrapper();

}

int main() {
    pthread_mutex_init(&mutex, NULL);
    connectToServer(SERVER_IP);
    pthread_mutex_destroy(&mutex);

    return 0;
}
