#ifndef LIBRARIAN_H
#define LIBRARIAN_H


#define MAX_NAME_LENGTH 50
#define MAX_EMAIL_LENGTH 50
#define MAX_PASSWORD_LENGTH 50
#define MAX_USERNAME_LENGTH 50


#include "../header/server.h"

typedef struct Librarian {               
    char username[MAX_USERNAME_LENGTH];                
    char name[MAX_NAME_LENGTH];
    char email[MAX_EMAIL_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    int LoginStatus;
} Librarian;


typedef struct BSTNodeLibrarian {
    struct Librarian data;
    struct BSTNodeLibrarian* left;
    struct BSTNodeLibrarian* right;
}BSTNodeLibrarian;


void librarianPacketHandler(int new_socket, MsgPacket *packet);
struct Librarian* createLibrarian(const char* username, const char* name, const char* email, const char* password);
struct BSTNodeLibrarian* createBSTNodeLibrarian(struct Librarian* librarian);
void insertLibrarian(struct BSTNodeLibrarian** root, struct Librarian librarian);
void displayLibrarians(struct BSTNodeLibrarian* root);
void ReadDatabaseLibrarian(struct BSTNodeLibrarian **root, const char *filename);
void librarianMenu(int new_socket, char *username);



#endif 