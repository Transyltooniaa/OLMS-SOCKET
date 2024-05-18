
#ifndef CLIENT_MENU_H
#define CLIENT_MENU_H


#define BUFFER_SIZE 1024
#define MAX_NAME_LENGTH 100


typedef struct MsgPacket {
    char *username;
    const char *role;
    const char **payload;
    int payload_count;
    int choice;
} MsgPacket;

// Function prototypes
void loginMenu(int sock);
void handleLogin(int sock, int choice);
void handleAuthentication(int sock,  char *username);
void borrowerMenu(int sock, char *username);
void librarianMenu(int sock, char *username);
void adminMenu(int sock, char *username);
void send_packet(int sock, MsgPacket *packet);
void clearInputBuffer();

#endif
