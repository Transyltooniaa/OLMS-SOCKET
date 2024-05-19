#include "../header/borrower.h"
#include "../header/book.h"
#include "../header/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/file.h> 
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_PASSWORD_LENGTH 20


int eligible =0;
int res = 0;
int validISBN = 0;
const char *eot;


//function to create a new borrower
struct Borrower createBorrower(int socket ,int ID, const char* username, const char* name, const char* password, long long int contact){
    struct Borrower borrower;
    borrower.ID = ID;
    strcpy(borrower.username, username);
    strcpy(borrower.name, name);
    strcpy(borrower.password, password);
    borrower.contact = contact;
    strcpy(borrower.borrowedBooks[0], "NULL");
    strcpy(borrower.borrowedBooks[1], "NULL");
    strcpy(borrower.borrowedBooks[2], "NULL");

    for (int i = 0; i < 3; i++) {
        borrower.borrowedBooksDueDate[i] = -1;
    }


    borrower.numBorrowedBooks = 0;
    borrower.fine = 0;
    borrower.isLate = 0;
    borrower.LoginStatus = 0;

    send(socket, "\t    Borrower created successfully !", strlen("\t    Borrower created successfully 1") + 1, 0);
    usleep(10000);
    send(socket, "END_OF_TRANSMISSION", strlen("END_OF_TRANSMISSION") + 1, 0);



    return borrower;


}



//function to create a new BST node
struct BSTNodeBorrower* createBSTNodeBorrower(struct Borrower* borrower) {
    struct BSTNodeBorrower* newNode = (struct BSTNodeBorrower*)malloc(sizeof(struct BSTNodeBorrower));
    if (newNode == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    newNode->data = *borrower;
    newNode->left = newNode->right = NULL;

    return newNode;
}


//function to insert a borrower into the BST on the basis of username
void insertBorrower(struct BSTNodeBorrower** root, struct Borrower borrower) {
    if (*root == NULL) {
        *root = createBSTNodeBorrower(&borrower);
        return;
    }

    if (strcmp(borrower.username, (*root)->data.username) < 0) {
        insertBorrower(&((*root)->left), borrower);
    } else {
        insertBorrower(&((*root)->right), borrower);
    }
}


void ReadDatabaseBorrower(struct BSTNodeBorrower **root, const char *filename) {
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
    struct Borrower borrower;

    while (fgets(buffer, BUFFER_SIZE, file) != NULL) {
        int numParsed = sscanf(buffer, "%d %49s %49s %49s %lld %49s %49s %49s %d %d %d %d %d %d %d",
                               &borrower.ID, borrower.username, borrower.name, borrower.password,
                               &borrower.contact, borrower.borrowedBooks[0], borrower.borrowedBooks[1],
                               borrower.borrowedBooks[2], &borrower.borrowedBooksDueDate[0],
                               &borrower.borrowedBooksDueDate[1], &borrower.borrowedBooksDueDate[2],
                               &borrower.numBorrowedBooks, &borrower.fine, &borrower.isLate,
                               &borrower.LoginStatus);
        if (numParsed != 15) {
            fprintf(stderr, "Error parsing line: %s", buffer);
            continue;  // Skip this line and continue with the next
        }

        // Insert the parsed borrower into the BST
        insertBorrower(root, borrower);
    }

    if (flock(fd, LOCK_UN) != 0) {
        perror("Error unlocking file");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fclose(file);
}





// Helper function to write the BST to a file with file locking
void writeBSTToFileHelperBorrower(struct BSTNodeBorrower *root, int fd) {
    if (root == NULL) {
        return;
    }

    writeBSTToFileHelperBorrower(root->left, fd);
    dprintf(fd, "%d %s %s %s %lld %s %s %s %d %d %d %d %d %d %d\n", root->data.ID, root->data.username, root->data.name, root->data.password, root->data.contact, root->data.borrowedBooks[0], root->data.borrowedBooks[1], root->data.borrowedBooks[2],root->data.borrowedBooksDueDate[0],root->data.borrowedBooksDueDate[1],root->data.borrowedBooksDueDate[2], root->data.numBorrowedBooks, root->data.fine, root->data.isLate, root->data.LoginStatus);
    writeBSTToFileHelperBorrower(root->right, fd);
}

// Function to write the database to a file with file locking
void WriteDatabaseBorrower(struct BSTNodeBorrower *root, const char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    struct flock fl;
    fl.l_type = F_WRLCK; 
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    // Acquire exclusive lock
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("Error locking file");
        exit(EXIT_FAILURE);
    }

    writeBSTToFileHelperBorrower(root, fd);

    // Release lock
    fl.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        perror("Error unlocking file");
        exit(EXIT_FAILURE);
    }

    close(fd);
}


//funtion to set the login status of a borrower
void setLoginStatus(struct BSTNodeBorrower *root, char *username, int status) {
    if (root == NULL) {
        return;
    }

    if (strcmp(root->data.username, username) == 0) {
        root->data.LoginStatus = status;
        WriteDatabaseBorrower(root, "../database/users/borrower.txt");
        return;
    }

    if (strcmp(username, root->data.username) < 0) {
        setLoginStatus(root->left, username, status);
    } else {
        setLoginStatus(root->right, username, status);
    }
}


void showBorrowedBooks(int socket, struct BSTNodeBorrower *root, MsgPacket *packet) {
    if (root == NULL) {
        return;
    }

    // Check if the current node's username matches the packet's username
    if (strcmp(root->data.username, packet->username) == 0) {
        char buffer[BUFFER_SIZE];
        for (int i = 0; i < 3; i++) {
            if (strcmp(root->data.borrowedBooks[i], "NULL") != 0) {
                snprintf(buffer, BUFFER_SIZE, "\tBOOK : %s\n", root->data.borrowedBooks[i]);
                send(socket, buffer, strlen(buffer) + 1, 0); 
                usleep(10000);
            }
        }
        send(socket, "END_OF_TRANSMISSION", strlen("END_OF_TRANSMISSION") + 1, 0);
        return;
    }

    if (strcmp(packet->username, root->data.username) < 0) {
        showBorrowedBooks(socket, root->left, packet);
    } else {
        showBorrowedBooks(socket, root->right, packet);
    }
}

// Wrapper function to call showBorrowedBooks and ensure "END_OF_TRANSMISSION" is sent once
void sendBorrowedBooks(int socket, struct BSTNodeBorrower *root, MsgPacket *packet) {
    showBorrowedBooks(socket, root, packet);
    send(socket, "END_OF_TRANSMISSION", strlen("END_OF_TRANSMISSION") + 1, 0);
}



void showMyInfo(int socket, struct BSTNodeBorrower *root, MsgPacket *packet) {
    if (root == NULL) {
        return;
    }

    if (strcmp(root->data.username, packet->username) == 0) {
        char buffer[BUFFER_SIZE];
        sprintf(buffer, "Name: %s\nContact: %lld\nFine: %d Rs\nLate: %d\nNumber of Books borrwed: %d\n ", root->data.name, root->data.contact, root->data.fine, root->data.isLate, root->data.numBorrowedBooks);
        send(socket, buffer, strlen(buffer), 0);
        usleep(10000);
        send(socket, "END_OF_TRANSMISSION", strlen("END_OF_TRANSMISSION") + 1, 0);
    }

    const char *endOfTransmission = "END_OF_TRANSMISSION";
    send(socket, endOfTransmission, strlen(endOfTransmission) + 1, 0);
}

    

void changePassword(int socket, struct BSTNodeBorrower *root, MsgPacket *packet) {
    if (root == NULL) {
        return;
    }

    if (strcmp(root->data.username, packet->username) == 0) {
        strcpy(root->data.password, packet->payload[0]);
        root->data.password[MAX_PASSWORD_LENGTH - 1] = '\0';
        WriteDatabaseBorrower(root, "../database/users/borrower.txt");
        send(socket, "\tPassword changed successfully", strlen("\tPassword changed successfully") + 1, 0);
        usleep(10000);
        send(socket, "END_OF_TRANSMISSION", strlen("END_OF_TRANSMISSION") + 1, 0);
        return;
    }

    if (strcmp(packet->username, root->data.username) < 0) {
        changePassword(socket, root->left, packet);
    } else {
        changePassword(socket, root->right, packet);
    }

    const char *endOfTransmission = "END_OF_TRANSMISSION";
    send(socket, endOfTransmission, strlen(endOfTransmission) + 1, 0);
}

void updateContact(int socket, struct BSTNodeBorrower *root, MsgPacket *packet) {
    if (root == NULL) {
        return;
    }

    if (strcmp(root->data.username, packet->username) == 0) {
        root->data.contact = atoll(packet->payload[0]);
        WriteDatabaseBorrower(root, "../database/users/borrower.txt");
        send(socket, "\tContact updated successfully", strlen("Contact updated successfully") + 1, 0);
        usleep(10000);
        send(socket, "END_OF_TRANSMISSION", strlen("END_OF_TRANSMISSION") + 1, 0);
        return;
    }

    if (strcmp(packet->username, root->data.username) < 0) {
        updateContact(socket, root->left, packet);
    } else {
        updateContact(socket, root->right, packet);
    }

    const char *endOfTransmission = "END_OF_TRANSMISSION";
    send(socket, endOfTransmission, strlen(endOfTransmission) + 1, 0);
}   

void checkDueDate(int socket, struct BSTNodeBorrower *root, MsgPacket *packet) {
    if (root == NULL) {
        return;
    }

    if (strcmp(root->data.username, packet->username) == 0) {
        char buffer[BUFFER_SIZE];
        if (root->data.isLate) {
            strcpy(buffer, "You have a fine of Rs. ");
            char fine[10];
            sprintf(fine, "%d", root->data.fine);
            strcat(buffer, fine);
            strcat(buffer, "\n");
        } else {
            strcpy(buffer, "You have no fine\n");
        }
        send(socket, buffer, strlen(buffer), 0);
        usleep(10000);
        send(socket, "END_OF_TRANSMISSION", strlen("END_OF_TRANSMISSION") + 1, 0);
        return;
    }

    if (strcmp(packet->username, root->data.username) < 0) {
        checkDueDate(socket, root->left, packet);
    } else {
        checkDueDate(socket, root->right, packet);
    }

    const char *endOfTransmission = "END_OF_TRANSMISSION";
    send(socket, endOfTransmission, strlen(endOfTransmission) + 1, 0);
}



void logout(int socket) {
    const char *msg = "     You have been logged out successfully\n\n";
    send(socket, msg, strlen(msg)+1, 0); 
    usleep(10000);
    const char *eot = "END_OF_TRANSMISSION";
    send(socket, eot, strlen(eot)+1, 0); 
}




void borrowBookUserUpdate(struct BSTNodeBorrower *root, char *username, const char *ISBN, struct BSTNodeBook *rootbook) {
    if (root == NULL) {
        return;
    }

    char bookName[MAX_TITLE_LENGTH];
    FetchBookNameFromISBN(rootbook, ISBN, bookName);

    if (strcmp(root->data.username, username) == 0) {
        for (int i = 0; i < 3; i++) {
            if (strcmp(root->data.borrowedBooks[i], "NULL") == 0) {
                strcpy(root->data.borrowedBooks[i], bookName);
                root->data.numBorrowedBooks++;
                root->data.borrowedBooksDueDate[i] = time(NULL) + 604800; 
                break;
            }
        }
        return;
    }

    if (strcmp(username, root->data.username) < 0) {
        borrowBookUserUpdate(root->left, username, ISBN, rootbook);
    } else {
        borrowBookUserUpdate(root->right, username, ISBN, rootbook);
    }

}      



int isEligibleToBorrow(struct BSTNodeBorrower *root, char *username) {
    if (root == NULL) {
        return 0;
    }

    if (strcmp(root->data.username, username) == 0) {
        if (root->data.numBorrowedBooks < 3) {
            return 1;
        } else {
            return 0;
        }
    }

    if (strcmp(username, root->data.username) < 0) {
        return isEligibleToBorrow(root->left, username);
    } else {
        return isEligibleToBorrow(root->right, username);
    }
}



int isEligibleTOReturn(struct BSTNodeBorrower *root, char *username, char *bookName) {
    if (root == NULL) {
        return 0;
    }

    if (strcmp(root->data.username, username) == 0) {
        for (int i = 0; i < 3; i++) {
            if (strcmp(root->data.borrowedBooks[i], bookName) == 0) {
                return 1;
            }
        }
        return 0;
    }

    if (strcmp(username, root->data.username) < 0) {
        return isEligibleTOReturn(root->left, username, bookName);
    } else {
        return isEligibleTOReturn(root->right, username, bookName);
    }
}



void returnBookUserUpdate(struct BSTNodeBorrower *root, char *username, const char *ISBN, struct BSTNodeBook *rootbook) {
    if (root == NULL) {
        return;
    }

    // Fetch the book name from ISBN
    char bookName[MAX_TITLE_LENGTH];
    FetchBookNameFromISBN(rootbook, ISBN, bookName);


    if (strcmp(root->data.username, username) == 0) {
        for (int i = 0; i < 3; i++) {
            if (strcmp(root->data.borrowedBooks[i], bookName) == 0) {
                strcpy(root->data.borrowedBooks[i], "NULL");
                root->data.numBorrowedBooks--;
                root->data.borrowedBooksDueDate[i] = -1;
                break;
            }
        }
        return;
    }

    if (strcmp(username, root->data.username) < 0) {
        returnBookUserUpdate(root->left, username, ISBN, rootbook);
    } else {
        returnBookUserUpdate(root->right, username, ISBN, rootbook);
    }
}

// Function to Show all the borrower and their details and send eot
void showAllBorrowers(int socket, struct BSTNodeBorrower *root) {
    if (root == NULL) {
        return;
    }

    showAllBorrowers(socket, root->left);

    char buffer[BUFFER_SIZE];
    sprintf(buffer, "Username: %s\nName: %s\nContact: %lld\nFine: %d\nLate: %d\nNumber of Books borrowed: %d\n\n", 
            root->data.username, root->data.name, root->data.contact, root->data.fine, root->data.isLate, root->data.numBorrowedBooks);
    send(socket, buffer, strlen(buffer), 0);
    usleep(10000);

    showAllBorrowers(socket, root->right);
    
    // Send the end of transmission message after the entire traversal
    if (root->left == NULL && root->right == NULL) {
        const char *eot = "END_OF_TRANSMISSION";
        send(socket, eot, strlen(eot) + 1, 0);
    }
}

// show all the borrower logged in
void showAllBorrowersLoggedIn(int socket, struct BSTNodeBorrower *root) {
    if (root == NULL) {
        return;
    }

    showAllBorrowersLoggedIn(socket, root->left);

    if (root->data.LoginStatus == 1) {
        char buffer[BUFFER_SIZE];
        sprintf(buffer, "Username: %s\nName: %s\nContact: %lld\nFine: %d\nLate: %d\nNumber of Books borrowed: %d\n\n", 
                root->data.username, root->data.name, root->data.contact, root->data.fine, root->data.isLate, root->data.numBorrowedBooks);
        send(socket, buffer, strlen(buffer), 0);
        usleep(10000);
        const char *eot = "END_OF_TRANSMISSION";
        send(socket, eot, strlen(eot) + 1, 0);
        usleep(10000);

        return;
    }

    showAllBorrowersLoggedIn(socket, root->right);
    // Send the end of transmission message after the entire traversal
    if (root->left == NULL && root->right == NULL) {
        send(socket,"\t    No borrowers logged in\n",strlen("\t    No borrowers logged in\n"),0);
        const char *eot = "END_OF_TRANSMISSION";
        send(socket, eot, strlen(eot) + 1, 0);
    }
    return;
}

// Find the minimum node in the BST
struct BSTNodeBorrower *FindMinBorrower(struct BSTNodeBorrower *root) {
    struct BSTNodeBorrower *current = root;
    while (current && current->left != NULL) {
        current = current->left;
    }
    return current;
}

// Delete a borrower from the BST and send message and EOT to the client
void deleteBorrower(int socket, struct BSTNodeBorrower **root, const char *username) {
    if (*root == NULL) {
        return;
    }

    if (strcmp(username, (*root)->data.username) < 0) {
        deleteBorrower(socket, &((*root)->left), username);
    } else if (strcmp(username, (*root)->data.username) > 0) {
        deleteBorrower(socket, &((*root)->right), username);
    } else {
        if ((*root)->left == NULL && (*root)->right == NULL) {
            free(*root);
            *root = NULL;
        } else if ((*root)->left == NULL) {
            struct BSTNodeBorrower *temp = *root;
            *root = (*root)->right;
            free(temp);
        } else if ((*root)->right == NULL) {
            struct BSTNodeBorrower *temp = *root;
            *root = (*root)->left;
            free(temp);
        } else {
            struct BSTNodeBorrower *temp = FindMinBorrower((*root)->right);
            (*root)->data = temp->data;
            deleteBorrower(socket, &((*root)->right), temp->data.username);
        }
        send(socket, "\tBorrower deleted successfully\n", strlen("\tBorrower deleted successfully\n"), 0);
        usleep(10000);
        send(socket, "END_OF_TRANSMISSION", strlen("END_OF_TRANSMISSION") + 1, 0);
    }
}



int getMaxUserID(struct BSTNodeBorrower *root) {
    if (root == NULL) {
        return 0;
    }

    int max = root->data.ID;
    int leftMax = getMaxUserID(root->left);
    int rightMax = getMaxUserID(root->right);

    if (leftMax > max) {
        max = leftMax;
    }

    if (rightMax > max) {
        max = rightMax;
    }

    return max;
}


int calculateRemainingDays(time_t dueDate) {
    time_t now = time(NULL);
    double seconds = difftime(dueDate, now);
    int days = (int)(seconds / (60 * 60 * 24));
    return days;
}

void sendDueDatesBorr(int socket, struct BSTNodeBorrower *root, MsgPacket *packet) {
    if (root == NULL) {
        return;
    }

    // Check if the current node's username matches the packet's username
    if (strcmp(root->data.username, packet->username) == 0) {
        char buffer[BUFFER_SIZE];
        for (int i = 0; i < 3; i++) {
            if (strcmp(root->data.borrowedBooks[i], "NULL") != 0) {
                int remainingTime = calculateRemainingDays(root->data.borrowedBooksDueDate[i]);
                snprintf(buffer, BUFFER_SIZE, "\t%s: %d days remaining\n", root->data.borrowedBooks[i], remainingTime);
                send(socket, buffer, strlen(buffer), 0);
                usleep(10000);
            }
        }
        send(socket, "END_OF_TRANSMISSION", strlen("END_OF_TRANSMISSION") + 1, 0);
        return;
    }

    if (strcmp(packet->username, root->data.username) < 0) {
        sendDueDatesBorr(socket, root->left, packet);
    } else {
        sendDueDatesBorr(socket, root->right, packet);
    }

    const char *endOfTransmission = "END_OF_TRANSMISSION";
    send(socket, endOfTransmission, strlen(endOfTransmission) + 1, 0);
}


void showBooksBeyondDueDate(int socket, struct BSTNodeBorrower *root, MsgPacket *packet) {
    static int booksOverdue = 0; // Static to track books across recursive calls
    static int isRootCall = 1;   // Static to track if the current call is the root call

    if (root == NULL) {
        return;
    }

    char buffer[BUFFER_SIZE];
    for (int i = 0; i < 3; i++) {
        if (strcmp(root->data.borrowedBooks[i], "NULL") != 0) {
            int remainingTime = calculateRemainingDays(root->data.borrowedBooksDueDate[i]);
            if (remainingTime < 0) {
                snprintf(buffer, BUFFER_SIZE, "%s: %d days overdue \nName: %s\nContact: %lld \nusername: %s\n\n", root->data.borrowedBooks[i], -remainingTime, root->data.name, root->data.contact, root->data.username);
                send(socket, buffer, strlen(buffer)+1, 0);
                usleep(10000);
                booksOverdue++;
            }
        }
    }
    
    // Taverse the left and right subtrees
    showBooksBeyondDueDate(socket, root->left, packet);
    showBooksBeyondDueDate(socket, root->right, packet);

    if (isRootCall) {
        if (booksOverdue == 0) {
            send(socket, "\t\tNo books are overdue!\n", strlen("\t\tNo books are overdue!\n")+1, 0);
        }
        const char *endOfTransmission = "END_OF_TRANSMISSION";
        send(socket, endOfTransmission, strlen(endOfTransmission) + 1, 0);
        
        booksOverdue = 0;
        isRootCall = 1;
    } else {
        isRootCall = 0;
    }
}




// Packet Handler
void borrowerPacketHandler(int new_socket, MsgPacket *packet)
{
    struct BSTNodeBorrower *rootborrower = NULL;
    ReadDatabaseBorrower(&rootborrower, "../database/users/borrower.txt");

    struct BSTNodeBook *rootbook = NULL;
    ReadDatabaseBook(&rootbook, "../database/Books/books.txt");


    setLoginStatus(rootborrower, packet->username, 1);

    switch (packet->choice)
    {
    case 1:
        printf("REQFROMCLIENT (ALL_GENRES) --- %s\n" , packet->username);
        ReadAllGenres(new_socket,rootbook ,packet);
        break;
    case 2:
        printf("REQFROMCLIENT (ALL_BOOKS) --- %s\n" , packet->username);
        ReadAllBooks(new_socket,rootbook ,packet);
        break;

    case 3:
        printf("REQFROMCLIENT (BORROW_BOOK) --- %s\n" , packet->username);
        validISBN = validateISBN(packet->payload[0]);
        if(validISBN == 0)
        {
            send(new_socket, "\t\tInvalid ISBN !", strlen("\t\tInvalid ISBN !") + 1, 0);
            usleep(10000);
            send(new_socket, "END_OF_TRANSMISSION", strlen("END_OF_TRANSMISSION") + 1, 0);
            break;
        }

        eligible = isEligibleToBorrow(rootborrower, packet->username);
        if(eligible == 0)
        {
            send(new_socket, "\tYou have already borrowed 3 books !", strlen("\tYou have already borrowed 3 books !") + 1, 0);
            usleep(10000);
            send(new_socket, "END_OF_TRANSMISSION", strlen("END_OF_TRANSMISSION") + 1, 0);
            break;
        }

        res = borrowBook(new_socket,rootbook ,packet->payload[0], packet->username);
        if(res == 1)
        {
            writeBSTToFileBook(rootbook, "../database/books/books.txt");
            borrowBookUserUpdate(rootborrower, packet->username, packet->payload[0],rootbook);
            WriteDatabaseBorrower(rootborrower, "../database/users/borrower.txt");
        }

        break;

    case 4:
        printf("REQFROMCLIENT (RETURN_BOOK) --- %s\nx" , packet->username);
        validISBN = validateISBN(packet->payload[0]);
        if(validISBN == 0)
        {
            send(new_socket, "\t\tInvalid ISBN !", strlen("\t\tInvalid ISBN !") + 1, 0);
            usleep(10000);
            send(new_socket, "END_OF_TRANSMISSION", strlen("END_OF_TRANSMISSION") + 1, 0);
            break;
        }

        // Getting the book name from ISBN
        char bookName[MAX_TITLE_LENGTH];
        FetchBookNameFromISBN(rootbook, packet->payload[0], bookName);


        eligible = isEligibleTOReturn(rootborrower, packet->username ,bookName);
        if(eligible == 0)
        {
            send(new_socket, "\tYou have not borrowed any such book !", strlen("\tYou have not borrowed any book !") + 1, 0);
            usleep(10000);
            send(new_socket, "END_OF_TRANSMISSION", strlen("END_OF_TRANSMISSION") + 1, 0);
            break;
        }

        res = returnBook(new_socket,rootbook ,packet->payload[0], packet->username);
        if(res == 1)
        {
            returnBookUserUpdate(rootborrower, packet->username, packet->payload[0],rootbook);
            writeBSTToFileBook(rootbook, "../database/books/books.txt");
            WriteDatabaseBorrower(rootborrower, "../database/users/borrower.txt");
        }

        break;
    
 
    case 5:
        printf("REQFROMCLIENT (SHOW_BORROWED_BOOKS) --- %s\n" , packet->username);
        showBorrowedBooks(new_socket,rootborrower ,packet);
        break;

    case 6:
        printf("REQFROMCLIENT (SHOW_MY_INFO) --- %s\n" , packet->username);
        showMyInfo(new_socket,rootborrower ,packet);
        break;

    case 7:
        printf("REQFROMCLIENT (CHANGE_PASSWORD) --- %s\n" , packet->username);
        changePassword(new_socket,rootborrower ,packet);
        break;

    case 8:
        printf("REQFROMCLIENT (UPDATE_CONTACT) --- %s\n" , packet->username);
        updateContact(new_socket,rootborrower ,packet);
        break;

    case 9:
        printf("REQFROMCLIENT (CHECK_DUE_DATE) --- %s\n" , packet->username);
        sendDueDatesBorr(new_socket,rootborrower ,packet);
        break;
    
    case 10:
        printf("REQFROMCLIENT (LOGOUT) --- %s\n" , packet->username);
        setLoginStatus(rootborrower, packet->username, 0);
        logout(new_socket);
        break;

    default:
        printf("REQFROMCLIENT (INVALID) --- %s\n" , packet->username);
        eot = "END_OF_TRANSMISSION";
        send(new_socket, eot, strlen(eot) + 1, 0);
        break;

    }
}
