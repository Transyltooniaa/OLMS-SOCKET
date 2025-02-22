#include "../header/librarian.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include "../header/borrower.h"
#include "../header/book.h"
#include <sys/file.h>
#include <unistd.h>


int isAvailable = 0;
struct Borrower borrower;


//function to create a new librarian
struct Librarian* createLibrarian(const char* username, const char* name, const char* email, const char* password) {
    struct Librarian* librarian = (struct Librarian*)malloc(sizeof(struct Librarian));
    if (librarian == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    strcpy(librarian->username, username);
    strcpy(librarian->name, name);
    strcpy(librarian->email, email);
    strcpy(librarian->password, password);
    librarian->LoginStatus = 0;

    return librarian;
}


//function to create a new BST node
struct BSTNodeLibrarian* createBSTNodeLibrarian(struct Librarian* librarian) {
    struct BSTNodeLibrarian* newNode = (struct BSTNodeLibrarian*)malloc(sizeof(struct BSTNodeLibrarian));
    if (newNode == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    newNode->data = *librarian;
    newNode->left = newNode->right = NULL;

    return newNode;
}


// Funtion to insert a librarian into the BST
void insertLibrarian(struct BSTNodeLibrarian** root, struct Librarian librarian) {
    if (*root == NULL) {
        *root = createBSTNodeLibrarian(&librarian);
        return;
    }

    if (strcmp(librarian.username, (*root)->data.username) < 0) {
        insertLibrarian(&((*root)->left), librarian);
    } else {
        insertLibrarian(&((*root)->right), librarian);
    }
}


// function to display all librarians in the BST
void displayLibrarians(struct BSTNodeLibrarian* root) {
    if (root == NULL) {
        return;
    }

    displayLibrarians(root->left);
    printf("Username: %s, Name: %s, Email: %s, LoginStatus: %d\n",
           root->data.username, root->data.name, root->data.email, root->data.LoginStatus);
    displayLibrarians(root->right);
}


// function to free the memory allocated for the BST
void freeLibrarianBST(struct BSTNodeLibrarian* root) {
    if (root == NULL) {
        return;
    }

    freeLibrarianBST(root->left);
    freeLibrarianBST(root->right);
    free(root);
}

// funtion to read the database from a file
void ReadDatabaseLibrarian(struct BSTNodeLibrarian **root, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int fd = fileno(file);
    if (flock(fd, LOCK_SH) != 0) {
        perror("Error locking file");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    struct Librarian librarian;

    while (fgets(buffer, BUFFER_SIZE, file) != NULL) {
        int numParsed = sscanf(buffer, "%49s %49s %49s %49s %d",
                               librarian.username, librarian.name, librarian.email,
                               librarian.password, &librarian.LoginStatus);
        if (numParsed != 5) {
            fprintf(stderr, "Error parsing line: %s", buffer);
            continue;  // Skip this line and continue with the next
        }

        // Insert the parsed librarian into the BST
        insertLibrarian(root, librarian);
    }

    if (flock(fd, LOCK_UN) != 0) {
        perror("Error unlocking file");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fclose(file);
}

void WriteLibrarianHelper(struct BSTNodeLibrarian *root, FILE *file) {
    if (root == NULL) {
        return;
    }

    WriteLibrarianHelper(root->left, file);
    fprintf(file, "%s %s %s %s %d\n", root->data.username, root->data.name, root->data.email, root->data.password, root->data.LoginStatus);
    WriteLibrarianHelper(root->right, file);
}

void lockFile(int fd, short lock_type) {
    struct flock lock;
    lock.l_type = lock_type; // F_RDLCK, F_WRLCK, F_UNLCK
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0; // 0 means lock the entire file

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Error setting lock");
        close(fd);
        exit(EXIT_FAILURE);
    }
}

void WriteLibrarian(struct BSTNodeLibrarian **root, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int fd = fileno(file);
    lockFile(fd, F_WRLCK); // Acquire an exclusive write lock

    WriteLibrarianHelper(*root, file);

    lockFile(fd, F_UNLCK); // Release the lock

    fclose(file);
}
// function to set the login status of a librarian
void setLoginStatusLibr(struct BSTNodeLibrarian* root, const char* username, int status) {
    if (root == NULL) {
        return;
    }

    if (strcmp(username, root->data.username) == 0) {
        root->data.LoginStatus = status;
        WriteLibrarian(&root, "../database/users/librarian.txt");
        return;
    }

    if (strcmp(username, root->data.username) < 0) {
        setLoginStatusLibr(root->left, username, status);
    } else {
        setLoginStatusLibr(root->right, username, status);
    }
}


void traverseAndSend(int socket, struct BSTNodeLibrarian* root, char* buffer, int* count) {
    if (root == NULL) {
        return;
    }

    traverseAndSend(socket, root->left, buffer, count);

    if (root->data.LoginStatus) {
        int len = snprintf(buffer, BUFFER_SIZE, "Username: %s, Name: %s, Email: %s, LoginStatus: %d\n",
                           root->data.username, root->data.name, root->data.email, root->data.LoginStatus);
        if (len < 0) {
            perror("snprintf failed");
            return;
        }
        if (send(socket, buffer, len, 0) == -1) {
            perror("send failed");
            return;
        }
        usleep(1000);
        (*count)++;
    }

    traverseAndSend(socket, root->right, buffer, count);
}

void showAllLibrariansLoggedIn(int socket, struct BSTNodeLibrarian* root) {
    char* buffer = (char*)malloc(BUFFER_SIZE);
    if (buffer == NULL) {
        perror("Failed to allocate buffer");
        return;
    }

    int count = 0;
    traverseAndSend(socket, root, buffer, &count);

    if (count == 0) {
        const char* msg = "No librarians logged in\n";
        if (send(socket, msg, strlen(msg), 0) == -1) {
            perror("send failed");
            free(buffer);
            return;
        }
        usleep(1000);
    }

    const char* eot = "END_OF_TRANSMISSION";
    if (send(socket, eot, strlen(eot), 0) == -1) {
        perror("send failed");
    }

    free(buffer);
}





//TODO: Implement the librarianPacketHandler function
void librarianPacketHandler(int new_socket, MsgPacket *packet) {

    int isAvailable = 0;
    int id = 0;

    struct BSTNodeBorrower *rootborrower = NULL;
    ReadDatabaseBorrower(&rootborrower, "../database/users/borrower.txt");

    struct BSTNodeBook *rootbook = NULL;
    ReadDatabaseBook(&rootbook, "../database/Books/books.txt");

    struct BSTNodeLibrarian *rootlibrarian = NULL;
    ReadDatabaseLibrarian(&rootlibrarian, "../database/users/librarian.txt");


    setLoginStatusLibr(rootlibrarian, packet->username, 1);



    switch(packet->choice)
    {
        case 1:
            printf("REQFROM CLIENT (ADD BOOK) --- %s\n", packet->username);
            isAvailable = atoi(packet->payload[5]) > 0 ? 1 : 0;
            insertBook(&rootbook,packet->payload[3] ,createBook(new_socket,packet->payload[0], packet->payload[2], packet->payload[1],atoi(packet->payload[5]) , isAvailable , atoi(packet->payload[4]) ,0 , 0 , "NULL")); 
            writeBSTToFileBook(rootbook, "../database/Books/books.txt");
            break;  

        case 2:
            printf("REQFROM CLIENT (DELETE BOOK) --- %s\n", packet->username);
            deleteBook(new_socket, &rootbook, packet->payload[0]);
            writeBSTToFileBook(rootbook, "../database/Books/books.txt");
            break;
            
        case 3:
            printf("REQFROM CLIENT (READ ALL BOOKS) --- %s\n", packet->username);
            ReadAllBooks(new_socket, rootbook, packet);
            break;

        case 4:
            printf("REQFROM CLIENT (READ ALL GENRES) --- %s\n", packet->username);
            ReadAllGenres(new_socket, rootbook, packet);
            break;

        case 5:
            printf("REQFROM CLIENT (READ ALL BORROWERS) --- %s\n", packet->username);
            showAllBorrowers(new_socket, rootborrower);
            break;

        case 6:
            printf("REQFROM CLIENT (ADD BORROWER) --- %s\n", packet->username);
            id = getMaxUserID(rootborrower) + 1;
            insertBorrower(&rootborrower, createBorrower(new_socket, id, packet->payload[2], packet->payload[0], packet->payload[1], atoll(packet->payload[3])));
            WriteDatabaseBorrower(rootborrower, "../database/users/borrower.txt");
            
            break;

        case 7:
            printf("REQFROM CLIENT (DELETE BORROWER) --- %s\n", packet->username);
            deleteBorrower(new_socket, &rootborrower, packet->payload[0]);
            WriteDatabaseBorrower(rootborrower, "../database/users/borrower.txt");
            break;

        case 8:
            printf("REQFROM CLIENT (UPDATE BORROWER) --- %s\n", packet->username);
            showAllBorrowersLoggedIn(new_socket, rootborrower);
            break;

        case 9:
            printf("REQFROM CLIENT (SHOW ALL LIBRARIANS LOGGED IN) --- %s\n", packet->username);
            showAllLibrariansLoggedIn(new_socket, rootlibrarian);
            break;

        case 10:
            printf("REQFROM CLIENT (LOGOUT) --- %s\n", packet->username);
            setLoginStatusLibr(rootlibrarian, packet->username, 0);
            logout(new_socket);
            break;

        case 11:
            packet->username = (char *)packet->payload[0];
            packet->payload[0] = packet->payload[1];
            printf("password: %s\n", packet->payload[0]);
            printf("REQFROM CLIENT (CHANGE BORROWER PASSWORD) --- %s\n", packet->username);
            changePassword(new_socket, rootborrower, packet);
            break;

        case 12:
            printf("REQFROM CLIENT TO SHOW ALL BOOKS BEYOND DUE DATE --- %s\n", packet->username);
            showBooksBeyondDueDate(new_socket, rootborrower, packet);
            break;

        default:
            break;
    }
}


    