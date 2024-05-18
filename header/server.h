// server.h

#ifndef SERVER_H
#define SERVER_H

#define PORT 8000
#define MAX_CLIENTS 250
#define WELCOME_MESSAGE "\n\nWelcome to the server of Online Library Management System\n"


#define BUFFER_SIZE 4096



typedef struct MsgPacket {
    char *username;
    const char *role;
    const char **payload;
    int payload_count;
    int choice;
} MsgPacket;




// Function prototypes
void startServer(int port);
void free_packet(MsgPacket *packet);
void send_packet(int new_socket, MsgPacket *packet);
void receive_packet(int new_socket, MsgPacket *packet);



#endif
