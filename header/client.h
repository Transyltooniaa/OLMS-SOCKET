#ifndef CLIENT_H
#define CLIENT_H

#define PORT 8000
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

// Function prototypes
void loginMenuWrapper();
void connectToServer(const char *server_ip);


#endif
