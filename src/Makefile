# Compiler to use
CC = cc

# Compiler flags
CFLAGS = -Wall -Wextra -Werror

# Source files for server and client
SERVER_SRCS = server.c auth.c books.c borrower.c librarian.c
CLIENT_SRCS = client.c clientMenu.c

# Executable names
SERVER_EXEC = server_exec
CLIENT_EXEC = client_exec

# Default target: compile both server and client
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Target to compile the server executable
$(SERVER_EXEC): $(SERVER_SRCS)
	$(CC) $(CFLAGS) $(SERVER_SRCS) -o $(SERVER_EXEC)

# Target to compile the client executable
$(CLIENT_EXEC): $(CLIENT_SRCS)
	$(CC) $(CFLAGS) $(CLIENT_SRCS) -o $(CLIENT_EXEC)

# Target to run the server
run_server: $(SERVER_EXEC)
	./$(SERVER_EXEC)

# Target to run the client
run_client: $(CLIENT_EXEC)
	./$(CLIENT_EXEC)

# Clean up build artifacts
clean:
	rm -f $(SERVER_EXEC) $(CLIENT_EXEC)

# Phony targets to avoid conflicts with files named 'all', 'clean', etc.
.PHONY: all clean run_server run_client
