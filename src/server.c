#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "../header/server.h"
#include "../header/auth.h"
#include "../header/borrower.h"
#include "../header/librarian.h"

int IsAuthenticated = 0;


void receive_packet(int sock, MsgPacket *packet) {
    
    char buffer[4096];  // Assuming the buffer is large enough
    int len = recv(sock, buffer, sizeof(buffer), 0);
    if (len <= 0) {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    buffer[len] = '\0';  // Ensure null-terminated string

    // Parse the serialized message
    packet->username = strdup(strtok(buffer, "|"));
    packet->role = strdup(strtok(NULL, "|"));
    packet->choice = atoi(strtok(NULL, "|"));
    packet->payload_count = atoi(strtok(NULL, "|"));

    // Allocate memory for the payload array
    packet->payload = malloc(packet->payload_count * sizeof(char *));
    if (packet->payload == NULL) {
        perror("Failed to allocate memory for payload");
        exit(EXIT_FAILURE);
    }

    // Parse the payload strings
    for (int i = 0; i < packet->payload_count; i++) {
        packet->payload[i] = strdup(strtok(NULL, "|"));
    }

}


void free_packet(MsgPacket *packet) {
    if (packet->username) {
        free((void*)packet->username);
    }
    if (packet->role) {
        free((void*)packet->role);
    }
    for (int i = 0; i < packet->payload_count; i++) {
        if (packet->payload[i]) {
            free((void*)packet->payload[i]);
        }
    }
    if (packet->payload) {
        free(packet->payload);
    }
}



void startServer(int port) {
    int server_fd, new_socket, valread;

    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    write(1, "Waiting for the client.....\n", sizeof("Waiting for the client.....\n"));

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        write(1, "Connected to the client.....\n", sizeof("Connected to the client.....\n"));
        send(new_socket, WELCOME_MESSAGE, strlen(WELCOME_MESSAGE), 0);
        int pid = fork();

        if (pid == 0) {
            close(server_fd);

            IsAuthenticated = authHandler(new_socket);
            MsgPacket packet;
            while (1) {
                receive_packet(new_socket, &packet);
                if(strcmp(packet.role, "borrower") == 0) {
                    borrowerPacketHandler(new_socket, &packet);
                } else if(strcmp(packet.role, "librarian") == 0) {
                    librarianPacketHandler(new_socket, &packet);
                } 
                // else if(strcmp(packet.role, "admin") == 0) {
                // //     adminPacketHandler(new_socket, packet);
                // // }



                free_packet(&packet);  // Free the packet after processing
            }

            close(new_socket);
            exit(0);
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else {
            close(new_socket);
        }
    }

    return;
}

int main() {
    startServer(PORT);
    return 0;
}

