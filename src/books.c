#include "../header/book.h"
#include "../header/borrower.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <fcntl.h> 
#include <unistd.h> 




void ReadAllGenres(int socket, struct BSTNodeBook *root, MsgPacket *packet) {
    if (root == NULL)
        return;

    ReadAllGenres(socket, root->left, packet);

    if (send(socket, root->genre.name, strlen(root->genre.name) + 1, 0) == -1) { 
        perror("send");
        return;
    }
    usleep(10000);

    ReadAllGenres(socket, root->right, packet);

    // Send end-of-transmission message
    if (root->left == NULL && root->right == NULL) {
        const char *endOfTransmission = "END_OF_TRANSMISSION";
        send(socket, endOfTransmission, strlen(endOfTransmission) + 1, 0);
    }
}




void ReadAllBooks(int socket,struct BSTNodeBook *root, MsgPacket *packet) {
    if (root == NULL)
        return;

    ReadAllBooks(socket, root->left, packet);

    char buffer[4096] = {0}; 
    for (int i = 0; i < root->genre.numBooks; i++) {
        struct LibraryBook *book = &(root->genre.books[i]);
        snprintf(buffer, sizeof(buffer), "BOOK: %s\nAUTHOR: %s\nISBN: %s\nCOPIES AVAILABLE: %d\nYEAR-PUBLISHED: %d\n\n", 
                 book->title, book->author, book->ISBN, book->numCopies,
                 book->yearPublished);
        
        if (send(socket, buffer, strlen(buffer), 0) == -1) { 
            perror("send");
            return;
        }
        send(socket, "\0", 1, 0);
        usleep(10000); 
    }

    ReadAllBooks(socket, root->right, packet);
    
    const char *endOfTransmission = "END_OF_TRANSMISSION";
    send(socket, endOfTransmission, strlen(endOfTransmission) + 1, 0);
}




// Function to create a new book
struct LibraryBook* createBook(int socket ,const char* title, const char* author, const char* ISBN, int numCopies, int isAvailable, int yearPublished, time_t issueDate, time_t returnDate,char *username) {
    struct LibraryBook* book = (struct LibraryBook*)malloc(sizeof(struct LibraryBook));
    if (book == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    strcpy(book->title, title);
    strcpy(book->author, author);
    strcpy(book->ISBN, ISBN);
    book->numCopies = numCopies;
    book->isAvailable = isAvailable;
    book->yearPublished = yearPublished;
    book->issueDate = issueDate;
    book->returnDate = returnDate;
    book->borrowerUsername = username;

    send(socket, "\t    Book added successfully !", strlen("\t .   Book added successfully !") + 1, 0);
    usleep(10000);
    const char *eot = "END_OF_TRANSMISSION";
    send(socket, eot, strlen(eot) + 1, 0);


    return book;
}


// Function to create a new BST node
struct BSTNodeBook* createBSTNodeBook(struct LibraryBook* book) {
    struct BSTNodeBook* newNode = (struct BSTNodeBook*)malloc(sizeof(struct BSTNodeBook));
    if (newNode == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    newNode->genre.numBooks = 0;
    strcpy(newNode->genre.name, "");
    newNode->left = newNode->right = NULL;

    return newNode;
}

// Function to insert a book into the BST
void insertBook(struct BSTNodeBook** root, const char* genreName, struct LibraryBook* book) {
    if (*root == NULL) {
        *root = createBSTNodeBook(book);
        strcpy((*root)->genre.name, genreName);
        (*root)->genre.books[0] = *book;
        (*root)->genre.numBooks++;
        return;
    }

    if (strcmp(genreName, (*root)->genre.name) < 0) {
        insertBook(&((*root)->left), genreName, book);
    } else if (strcmp(genreName, (*root)->genre.name) > 0) {
        insertBook(&((*root)->right), genreName, book);
    } else {
        (*root)->genre.books[(*root)->genre.numBooks] = *book;
        (*root)->genre.numBooks++;
    }
}



// Function to display all books in the BST
void displayAllBooks(struct BSTNodeBook* root) {
    if (root == NULL) {
        return;
    }

    displayAllBooks(root->left);

    printf("Genre: %s\n", root->genre.name);
    for (int i = 0; i < root->genre.numBooks; i++) {
        struct LibraryBook* book = &(root->genre.books[i]);
        printf("Title: %s\n", book->title);
        printf("Author: %s\n", book->author);
        printf("ISBN: %s\n", book->ISBN);
        printf("Number of copies: %d\n", book->numCopies);
        printf("Availability: %s\n", book->isAvailable ? "Yes" : "No");
        printf("Year Published: %d\n", book->yearPublished);
        printf("Issue Date: %s", ctime(&book->issueDate));
        printf("Return Date: %s", ctime(&book->returnDate));
        printf("Borrower Username: %s\n", book->borrowerUsername);
        printf("\n");
    }

    displayAllBooks(root->right);
}


// Helper function to read a string enclosed in double quotes
void ReadDatabaseBook(struct BSTNodeBook **root, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char genreName[MAX_GENRE_LENGTH];
    struct LibraryBook book;

    while (fscanf(file, "%s", genreName) == 1) {
        fscanf(file, "%s", book.title);
        fscanf(file, "%s", book.author);
        fscanf(file, "%s", book.ISBN);
        fscanf(file, "%d", &book.numCopies);
        fscanf(file, "%d", &book.isAvailable);
        fscanf(file, "%d", &book.yearPublished);
        fscanf(file, "%ld", &book.issueDate);
        fscanf(file, "%ld", &book.returnDate);
        char borrowerID[MAX_BORROWER_ID_LENGTH];
        fscanf(file, "%s", borrowerID);
        book.borrowerUsername = strdup(borrowerID);

        insertBook(root, genreName, &book);
    }

    fclose(file);
}




// Helper function to perform in-order traversal and write to file with file locking
void writeBSTToFileHelperBook(struct BSTNodeBook *root, int fd) {
    if (root == NULL)
        return;

    writeBSTToFileHelperBook(root->left, fd);

    // Write the current node to the file
    for (int i = 0; i < root->genre.numBooks; i++) {
        struct LibraryBook *book = &(root->genre.books[i]);
        dprintf(fd, "%s %s %s %s %d %d %d %ld %ld %s\n", 
                root->genre.name, book->title, book->author, 
                book->ISBN, book->numCopies, book->isAvailable, 
                book->yearPublished, book->issueDate, book->returnDate, book->borrowerUsername);
    }

    writeBSTToFileHelperBook(root->right, fd);
}


// Function to write BST to file with file locking
void writeBSTToFileBook(struct BSTNodeBook *root, const char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        printf("Error opening file.\n");
        return;
    }

    struct flock fl;
    fl.l_type = F_WRLCK; // Exclusive write lock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    // Acquire exclusive lock
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("Error locking file");
        exit(EXIT_FAILURE);
    }

    writeBSTToFileHelperBook(root, fd);

    // Release lock
    fl.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        perror("Error unlocking file");
        exit(EXIT_FAILURE);
    }

    close(fd);
}




int borrowBook(int socket, struct BSTNodeBook *root, const char *ISBN, char *username) {

    if (root == NULL) 
        return 0;


    if (borrowBook(socket, root->left, ISBN, username)) {
        return 1;
    }

    for (int i = 0; i < root->genre.numBooks; i++) 
    {
        struct LibraryBook *book = &(root->genre.books[i]);

        if (strcmp(book->ISBN, ISBN) == 0) 
        {
            if (book->numCopies == 0)
            {
                book->isAvailable = 0;
                send(socket, "\t\tBook not available !", strlen("\t\tBook not available !") + 1, 0);
            } 
            
            else 
            {
                send(socket, "\t    Book borrowed successfully !", strlen("\t    Book borrowed successfully !") + 1, 0);
                book->numCopies--;
                book->isAvailable = book->numCopies > 0;
                book->issueDate = time(NULL);
                book->returnDate = book->issueDate + 604800; // 7 days
                strncpy(book->borrowerUsername, username, sizeof(book->borrowerUsername) - 1);
                book->borrowerUsername[sizeof(book->borrowerUsername) - 1] = '\0';
            }

            usleep(10000);
            const char *eot = "END_OF_TRANSMISSION";
            send(socket, eot, strlen(eot) + 1, 0);
            usleep(10000);
            return 1;
        }
    }


    if (borrowBook(socket, root->right, ISBN, username)) {
        return 1;
    }


    if(root->left == NULL && root->right == NULL){
        send(socket, "\t\tBook not found !", strlen("\t\tBook not found !") + 1, 0);
        usleep(10000);
        const char *eot = "END_OF_TRANSMISSION";
        send(socket, eot, strlen(eot) + 1, 0);
        return 0;
    }

    return 0;
 
}


void FetchBookNameFromISBN(struct BSTNodeBook *root, const char *ISBN, char *bookName) {
    if (root == NULL) {
        return;
    }

    FetchBookNameFromISBN(root->left, ISBN, bookName);

    for (int i = 0; i < root->genre.numBooks; i++) {
        struct LibraryBook *book = &(root->genre.books[i]);
        if (strcmp(book->ISBN, ISBN) == 0) {
            strcpy(bookName, book->title);
            return;
        }
    }

    FetchBookNameFromISBN(root->right, ISBN, bookName);
}



int CheckRemainingTimeForBookReturn(struct BSTNodeBook *root, const char *bookName) {
    if (root == NULL) {
        return INT_MAX;
    }

    int left = CheckRemainingTimeForBookReturn(root->left, bookName);

    for (int i = 0; i < root->genre.numBooks; i++) {
        struct LibraryBook *book = &(root->genre.books[i]);
        if (strcmp(book->title, bookName) == 0) {
            time_t currentTime = time(NULL);
            int remainingTime = (int)((book->returnDate - currentTime) / 86400); // Convert seconds to days
            return remainingTime;
        }
    }

    int right = CheckRemainingTimeForBookReturn(root->right, bookName);

    return left < right ? left : right;
}


int returnBook(int socket, struct BSTNodeBook *root, const char *ISBN, char *username) {
    if (root == NULL)     
        return 0;
    

    for (int i = 0; i < root->genre.numBooks; i++) {
        struct LibraryBook *book = &(root->genre.books[i]);
        if (strcmp(book->ISBN, ISBN) == 0) {
            send(socket, "\t   Book returned successfully !", strlen("\t   Book returned successfully !") + 1, 0);
            usleep(10000);
            book->numCopies++;
            book->isAvailable = 1;
            book->issueDate = 0;
            book->returnDate = 0;
            book->borrowerUsername = "NULL";
            const char *eot = "END_OF_TRANSMISSION";
            send(socket, eot, strlen(eot) + 1, 0);
            return 1;
        }

    }

    if (returnBook(socket, root->left, ISBN, username)) {
        return 1;
    }

    if (returnBook(socket, root->right, ISBN, username)) {
        return 1;
    }

    if(root->left == NULL && root->right == NULL){
        send(socket, "\t\tBook not found !", strlen("\t\tBook not found !") + 1, 0);
        usleep(10000);
        const char *eot = "END_OF_TRANSMISSION";
        send(socket, eot, strlen(eot) + 1, 0);
        return 0;
    }
    return 0;
}



int validateISBN(const char *ISBN) {
    if (strlen(ISBN) != 13) {
        return 0;
    }

    int sum = 0;
    for (int i = 0; i < 12; i++) {
        sum += (i % 2 == 0) ? (ISBN[i] - '0') : 3 * (ISBN[i] - '0');
    }

    int checksum = 10 - (sum % 10);
    if (checksum == 10) {
        checksum = 0;
    }

    return checksum == (ISBN[12] - '0');
}

// Delete a book from the BST given the ISBN , send a message to the client 
int deleteBookFromGenre(int socket, struct Genre *genre, const char *ISBN) {
    for (int i = 0; i < genre->numBooks; i++) {
        struct LibraryBook *book = &(genre->books[i]);
        if (strcmp(book->ISBN, ISBN) == 0) {
            send(socket, "\t    Book deleted successfully !", strlen("\t    Book deleted successfully !") + 1, 0);
            usleep(10000);
            const char *eot = "END_OF_TRANSMISSION";
            send(socket, eot, strlen(eot) + 1, 0);
            for (int j = i; j < genre->numBooks - 1; j++) {
                genre->books[j] = genre->books[j + 1];
            }
            genre->numBooks--;
            return 1;
        }
    }
    return 0;
}

int deleteBook(int socket, struct BSTNodeBook **root, const char *ISBN) {
    if (*root == NULL) {
        return 0;
    }

    if (deleteBook(socket, &((*root)->left), ISBN)) {
        return 1;
    }

    if (deleteBookFromGenre(socket, &((*root)->genre), ISBN)) {
        return 1;
    }

    if (deleteBook(socket, &((*root)->right), ISBN)) {
        return 1;
    }

    if ((*root)->left == NULL && (*root)->right == NULL) {
        send(socket, "\t\tBook not found !", strlen("\t\tBook not found !") + 1, 0);
        usleep(10000);
        const char *eot = "END_OF_TRANSMISSION";
        send(socket, eot, strlen(eot) + 1, 0);
    }

    return 0;
}

void updateBook(int socket, struct BSTNodeBook *root, const char *ISBN, const char *title, const char *author, const char *genre, const char *year, const char *quantity) {
    if (root == NULL) {
        return;
    }

    updateBook(socket, root->left, ISBN, title, author, genre, year, quantity);

    for (int i = 0; i < root->genre.numBooks; i++) {
        struct LibraryBook *book = &(root->genre.books[i]);
        if (strcmp(book->ISBN, ISBN) == 0) {
            strcpy(book->title, title);
            strcpy(book->author, author);
            strcpy(root->genre.name, genre);
            book->yearPublished = atoi(year);
            book->numCopies = atoi(quantity);
            book->isAvailable = book->numCopies > 0;
            send(socket, "\t    Book updated successfully !", strlen("\t    Book updated successfully !") + 1, 0);
            usleep(10000);
            const char *eot = "END_OF_TRANSMISSION";
            send(socket, eot, strlen(eot) + 1, 0);
            return;
        }
    }

    updateBook(socket, root->right, ISBN, title, author, genre, year, quantity);
}