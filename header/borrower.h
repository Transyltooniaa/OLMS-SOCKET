#ifndef BORROWER_H
#define BORROWER_H

#define MAX_NAME_LENGTH 50
#define MAX_ID_LENGTH 50
#define MAX_CONTACT_LENGTH 20


#include "server.h"


struct LibraryBook; // Forward declaration of LibraryBook struct

typedef struct Borrower {
    int ID;
    char username[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    char password[MAX_NAME_LENGTH]; 
    long long int contact;
    char borrowedBooks[3][MAX_NAME_LENGTH];
    int numBorrowedBooks;
    int fine;
    int isLate;
    int LoginStatus;
} Borrower;

// BST node structure
struct BSTNodeBorrower {

    struct Borrower data;
    struct BSTNodeBorrower* left;
    struct BSTNodeBorrower* right;
};

// Function prototypes
struct Borrower createBorrower(int socket ,int ID, const char* username, const char* name, const char* password, long long int contact);
struct BSTNodeBorrower* createBSTNodeBorrower(struct Borrower* borrower);
void insertBorrower(struct BSTNodeBorrower** root, struct Borrower borrower);
void ReadDatabaseBorrower(struct BSTNodeBorrower  **root, const char *filename);
void writeBSTToFileHelperBorrower(struct BSTNodeBorrower *root, int fd) ;
void WriteDatabaseBorrower(struct BSTNodeBorrower *root, const char *filename);
void setLoginStatus(struct BSTNodeBorrower *root, char *username, int status);
void showBorrowedBooks(int socket, struct BSTNodeBorrower *root, MsgPacket *packet);
void sendBorrowedBooks(int socket, struct BSTNodeBorrower *root, MsgPacket *packet);
void showMyInfo(int socket, struct BSTNodeBorrower *root, MsgPacket *packet);
void changePassword(int socket, struct BSTNodeBorrower *root, MsgPacket *packet);
void updateContact(int socket, struct BSTNodeBorrower *root, MsgPacket *packet);
void checkDueDate(int socket, struct BSTNodeBorrower *root, MsgPacket *packet);
void logout(int socket);
int isEligibleToBorrow(struct BSTNodeBorrower *root, char *username) ;
int isEligibleTOReturn(struct BSTNodeBorrower *root, char *username, char *bookName) ;
void showAllBorrowers(int socket, struct BSTNodeBorrower *root);
void borrowerPacketHandler(int new_socket, MsgPacket *packet);
void showAllBorrowersLoggedIn(int socket, struct BSTNodeBorrower *root);
void deleteBorrower(int socket ,struct BSTNodeBorrower **root, const char *username);
int getMaxUserID(struct BSTNodeBorrower *root);
#endif /* BORROWER_H */