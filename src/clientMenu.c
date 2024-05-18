#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "../header/clientMenu.h"

#define bufferMsg_SIZE 4096
#define MAX_NAME_LENGTH 100

char bufferMsg[bufferMsg_SIZE] = {0};





void send_packet(int sock, MsgPacket *packet) {
    int len = strlen(packet->username) + strlen(packet->role) + sizeof(int) * 2;
    for (int i = 0; i < packet->payload_count; ++i) {
        len += strlen(packet->payload[i]) + 1;
    }

    char *bufferMsg = malloc(len + 1);
    if (!bufferMsg) {
        perror("Failed to allocate memory for bufferMsg");
        exit(EXIT_FAILURE);
    }

    // Construct the message
    int pos = sprintf(bufferMsg, "%s|%s|%d|%d", packet->username, packet->role, packet->choice, packet->payload_count);
    for (int i = 0; i < packet->payload_count; ++i) {
        pos += sprintf(bufferMsg + pos, "|%s", packet->payload[i]);
    }

    send(sock, bufferMsg, len + 1, 0);
    free(bufferMsg);
}




void loginMenu(int sock) {
    int choice;

    printf("\n----------------Login Menu----------------\n\n");
    printf("1.) Borrower\n");
    printf("2.) Librarian\n");
    printf("3.) Admin\n");
    printf("4.) Exit application\n\n");
    printf("Enter your choice: ");
    scanf("%d", &choice);

    if (choice == 4) {
        printf("Exiting application...\n");
        printf("Thank you for using our application.\nTake care!\n");
        sleep(1);
        exit(0);
    }

    handleLogin(sock, choice);
}

void handleLogin(int sock, int choice) {
    char username[MAX_NAME_LENGTH];
    char password[MAX_NAME_LENGTH];

    printf("\n\n----------------Auth Menu----------------\n");
    printf("\nEnter Username: ");
    scanf("%s", username);
    printf("Enter Password: ");
    scanf("%s", password);

    send(sock, username, strlen(username), 0);
    usleep(100000);  // Sleep for 100 milliseconds
    send(sock, password, strlen(password), 0);
    usleep(100000);

    const char *role;
    switch (choice) {
        case 1:
            role = "borrower";
            break;
        case 2:
            role = "librarian";
            break;
        case 3:
            role = "admin";
            break;
        default:
            fprintf(stderr, "\t    \nInvalid choice\n");
            return;
    }

    send(sock, role, strlen(role), 0);
    handleAuthentication(sock, username);
}

void handleAuthentication(int sock, char *username) {
    char message[bufferMsg_SIZE];

    read(sock, message, bufferMsg_SIZE);

    if (strcmp(message, "Authenticated") == 0) {
        printf("\n\t    Login Successful !\n\n\n");

        if (username[0] == 'L') {
            librarianMenu(sock, username);
        } else if (username[0] == 'A') {
            adminMenu(sock, username);
        } else {
            borrowerMenu(sock, username);
        }
    } else {
        printf("\n\tLogin Failed! Please re-run the application\n\n\n");
    }
}



void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}


void borrowerMenu(int sock, char *username) {
    char bufferMsg[BUFFER_SIZE];
    
    while(1) {
        int choice;
        printf("----------------Borrower Menu----------------\n");
        printf("\n1.) Show all genres\n");
        printf("2.) Show all books\n");
        printf("3.) Borrow a book\n");
        printf("4.) Return a book\n");
        printf("5.) Show borrowed books\n");
        printf("6.) My Details\n");
        printf("7.) Change Password\n");
        printf("8.) Update Contact\n");
        printf("9.) Check Due Dates\n");
        printf("10.) Logout\n");

        printf("\nEnter your choice (IN NUMBERS): ");
        if (scanf("%d", &choice) != 1) {
            printf("\nInvalid input! PLEASE ENTER A NUMBER ONLY!\n\n\n");
            sleep(1);
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        const char *payload[1] = {NULL};

        if (choice == 3 || choice == 4) {
            char isbn[MAX_NAME_LENGTH];
            printf("\nEnter the ISBN of the book: ");
            scanf("%99s", isbn); // Limit input to avoid buffer overflow
            clearInputBuffer();
            payload[0] = strdup(isbn);
        } else if (choice == 7) {
            char new_password[MAX_NAME_LENGTH];
            printf("\nEnter your new password: ");
            scanf("%99s", new_password); // Limit input to avoid buffer overflow
            clearInputBuffer();
            payload[0] = strdup(new_password);
        } else if (choice == 8) {
            char new_contact[MAX_NAME_LENGTH];
            printf("\nEnter your new contact number: ");
            scanf("%99s", new_contact); // Limit input to avoid buffer overflow
            clearInputBuffer();
            payload[0] = strdup(new_contact);
        }

        MsgPacket packet = {
            .username = username,
            .role = "borrower",
            .payload = payload[0] ? payload : NULL,
            .payload_count = payload[0] ? 1 : 0,
            .choice = choice
        };

        send_packet(sock, &packet);
        printf("\n\n-----------RECEIVED MESSAGE FROM THE SERVER-----------\n\n");

        switch(choice) {
            case 1:
                printf("\n\t\tAll genres:\n\n");
                break;
            case 2:
                printf("\n\t\tAll books:\n\n");
                break;
            case 3:
                printf("\n\t\tBorrow a book:\n\n");
                break;
            case 4:
                printf("\n\t\tReturn a book:\n\n");
                break;
            case 5:
                printf("\n\t\tBorrowed books:\n\n");
                break;
            case 6:
                printf("\n\t\tMy details:\n\n");
                break;
            case 7:
                printf("\n\t\tChange password:\n\n");
                break;
            case 8:
                printf("\n\t\tUpdate contact:\n\n");
                break;
            case 9:
                printf("\n\t\tCheck due dates:\n\n");
                break;
            case 10:
                printf("\n\t\tLogging out...\n\n");
                break;
            default:
                printf("\n\t\tInvalid choice\n\n");
                break;
        }


        while (1) {
            ssize_t bytesRead = read(sock, bufferMsg, BUFFER_SIZE);
            if (bytesRead < 0) {
                perror("recv");
                break;
            }

            if (strcmp(bufferMsg, "END_OF_TRANSMISSION") == 0) {
                break;
            }
           
            if (strlen(bufferMsg) > 0) {
                printf("%s\n", bufferMsg);
            }
            
            memset(bufferMsg, 0, BUFFER_SIZE);
        }
        
        if (payload[0]) {
            free((void*)payload[0]);
        }

        if (choice == 10) {
            break;
        }

        printf("\n\n");


        usleep(500000); 
    }
}


void librarianMenu(int sock, char *username) {
    char bufferMsg[BUFFER_SIZE];

    while (1) {
        int choice;

        printf("\n----------------Librarian Menu----------------\n");
        printf("1.) Add a book\n");
        printf("2.) Remove a book\n");
        printf("3.) Update a book\n");
        printf("4.) Show all books\n");
        printf("5.) Show all genres\n");
        printf("6.) Show all borrowers\n");
        printf("7.) Add a borrower\n");
        printf("8.) Remove a borrower\n");
        printf("9.) Show all loggedIn Borrower\n");
        printf("10.) Show all loggedIn Librarian\n");
        printf("11.) Logout\n");

        printf("\nEnter your choice (IN NUMBERS): ");
        if (scanf("%d", &choice) != 1) {
            printf("\nInvalid input! PLEASE ENTER A NUMBER ONLY!\n\n\n");
            sleep(1);
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        char *payload[6] = {NULL}; // Array to hold payload elements, max 6 elements for Add a book

        if (choice == 1) {
            char title[MAX_NAME_LENGTH];
            char isbn[MAX_NAME_LENGTH];
            char author[MAX_NAME_LENGTH];
            char genre[MAX_NAME_LENGTH];
            char yearPublished[MAX_NAME_LENGTH];
            char numCopies[MAX_NAME_LENGTH];

            printf("\nEnter the title of the book: ");
            scanf("%99s", title);
            printf("\nEnter the ISBN of the book: ");
            scanf("%99s", isbn);
            printf("\nEnter the author of the book: ");
            scanf("%99s", author);
            printf("\nEnter the genre of the book: ");
            scanf("%99s", genre);
            printf("\nEnter the year published: ");
            scanf("%99s", yearPublished);
            clearInputBuffer();
            printf("\nEnter the number of copies: ");
            scanf("%99s", numCopies);
            clearInputBuffer();

            payload[0] = strdup(title);
            payload[1] = strdup(isbn);
            payload[2] = strdup(author);
            payload[3] = strdup(genre);
            payload[4] = strdup(yearPublished);
            payload[5] = strdup(numCopies);
        } else if (choice == 2 || choice == 3) {
            char isbn[MAX_NAME_LENGTH];
            printf("\nEnter the ISBN of the book: ");
            scanf("%99s", isbn);
            clearInputBuffer();
            payload[0] = strdup(isbn);
        }

        else if (choice == 7) {
            char name[MAX_NAME_LENGTH];
            char password[MAX_NAME_LENGTH];
            char borrowerUsername[MAX_NAME_LENGTH];
            char contact[MAX_NAME_LENGTH];

            printf("\nEnter the name of the borrower: ");
            scanf("%99s", name);
            printf("\nEnter the password of the borrower: ");
            scanf("%99s", password);
            printf("\nEnter the username of the borrower: ");
            scanf("%99s", borrowerUsername);
            printf("\nEnter the contact of the borrower: ");
            scanf("%99s", contact);
            clearInputBuffer();

            payload[0] = strdup(name);
            payload[1] = strdup(password);
            payload[2] = strdup(borrowerUsername);
            payload[3] = strdup(contact);
        }

        else if (choice == 8) {
            char borrowerUsername[MAX_NAME_LENGTH];
            printf("\nEnter the username of the borrower: ");
            scanf("%99s", borrowerUsername);
            clearInputBuffer();
            payload[0] = strdup(borrowerUsername);
        }

        else if (choice == 3) {
            char title[MAX_NAME_LENGTH];
            char isbn[MAX_NAME_LENGTH];
            char author[MAX_NAME_LENGTH];
            char genre[MAX_NAME_LENGTH];
            char yearPublished[MAX_NAME_LENGTH];
            char numCopies[MAX_NAME_LENGTH];

            printf("\nEnter the title of the book: ");
            scanf("%99s", title);
            printf("\nEnter the ISBN of the book: ");
            scanf("%99s", isbn);
            printf("\nEnter the author of the book: ");
            scanf("%99s", author);
            printf("\nEnter the genre of the book: ");
            scanf("%99s", genre);
            printf("\nEnter the year published: ");
            scanf("%99s", yearPublished);
            clearInputBuffer();
            printf("\nEnter the number of copies: ");
            scanf("%99s", numCopies);
            clearInputBuffer();

            payload[0] = strdup(title);
            payload[1] = strdup(isbn);
            payload[2] = strdup(author);
            payload[3] = strdup(genre);
            payload[4] = strdup(yearPublished);
            payload[5] = strdup(numCopies);
        }

        MsgPacket packet = {
            .username = username,
            .role = "librarian",
            .payload = payload[0] ? (const char **)payload : NULL,
            .payload_count = payload[0] ? (choice == 1 ? 6 : (choice == 7 ? 4 : 1)) : 0,
            .choice = choice
        };



        send_packet(sock, &packet);
        printf("\n\n-----------RECEIVED MESSAGE FROM THE SERVER-----------\n\n");

        switch (choice) {
            case 1:
                printf("\n\t\tAdding book:\n\n");
                break;
            case 2:
                printf("\n\t\tRemoving book:\n\n");
                break;
            case 3:
                printf("\n\t\tUpdating book copies\n\n");
                break;
            case 4:
                printf("\n\t\tShowing all books:\n\n");
                break;
            case 5:
                printf("\n\t\tShowing all genres\n\n");
                break;
            case 6:
                printf("\n\t\tShowing all borrowers:\n\n");
                break;
            case 7:
                printf("\n\t\tAdding borrower:\n\n");
                break;
            case 8:
                printf("\n\t\tRemoving borrower:\n\n");
                break;
            case 9:
                printf("\n\t\tLogged In Users\n\n");
                break;
            case 10:
                printf("\n\t\tLogged In Librarians\n\n");
                break;
            case 11:
                printf("\n\t\tLogging out...\n\n");
                break;
            default:
                printf("\n\t\tInvalid choice\n\n");
                break;
        }

        while (1) {
            ssize_t bytesRead = read(sock, bufferMsg, BUFFER_SIZE);
            if (bytesRead < 0) {
                perror("recv");
                break;
            }

            if (strcmp(bufferMsg, "END_OF_TRANSMISSION") == 0) {
                break;
            }

            if (strlen(bufferMsg) > 0) {
                printf("%s\n", bufferMsg);
            }

            memset(bufferMsg, 0, BUFFER_SIZE);
        }

        for (int i = 0; i < 6; ++i) {
            if (payload[i]) {
                free(payload[i]);
            }
        }

        if (choice == 11) {
            break;
        }

        printf("\n\n");
        usleep(500000);
    }
}




void adminMenu(int sock, char *username) {
    while (1) {
        int choice;

        printf("----------------Admin Menu----------------\n");
        printf("1.) Add a book\n");
        printf("2.) Remove a book\n");
        printf("3.) Update a book\n");
        printf("4.) Show all books\n");
        printf("5.) Show all genres\n");
        printf("6.) Show all borrowers\n");
        printf("7.) Show all borrowed books\n");
        printf("8.) Show all late books\n");
        printf("9.) Show all fines\n");
        printf("10.) Add a Librarian\n");
        printf("11.) Remove a Librarian\n");
        printf("12.) Update a Librarian\n");
        printf("13.) Show all Librarians\n");
        printf("14.) Logout\n");

        printf("Enter your choice: ");
        scanf("%d", &choice);

        const char *payload[1] = {NULL};

        if (choice == 1) {
            char title[MAX_NAME_LENGTH];
            printf("Enter the title of the book: ");
            scanf("%s", title);
            payload[0] = strdup(title);
        } else if (choice == 2 || choice == 3) {
            char isbn[MAX_NAME_LENGTH];
            printf("Enter the ISBN of the book: ");
            scanf("%s", isbn);
            payload[0] = strdup(isbn);
        }

        MsgPacket packet = {
            .username = username,
            .role = "admin",
            .payload = payload[0] ? payload : NULL,
            .payload_count = payload[0] ? 1 : 0,
            .choice = choice
        };

        send_packet(sock, &packet);

        while(1)
        {
            ssize_t bytesRead = read(sock, bufferMsg, bufferMsg_SIZE);
            if (bytesRead < 0) {
                perror("recv");
                break;
            } else if (bytesRead == 0) {
                break;
            } else if (bufferMsg[0] == '\0') {
                break;
            }

            printf("Received genre: %s\n", bufferMsg);
            memset(bufferMsg, 0, bufferMsg_SIZE);
        }

        // Free dynamically allocated payload strings
        if (payload[0]) {
            free((void*)payload[0]);
        }
    }
}

