CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -pthread -fsanitize=address
TARGET_DIR=.

# Targets
SERVER=server
CLIENT=client

# Source files
SERVER_SRC=server.c
CLIENT_SRC=client.c

# Default target
all: $(SERVER) $(CLIENT)

# Build server
$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(TARGET_DIR)/$(SERVER) $(SERVER_SRC)

# Build client
$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(TARGET_DIR)/$(CLIENT) $(CLIENT_SRC)

# Clean compiled files
clean:
	rm -f $(TARGET_DIR)/$(SERVER) $(TARGET_DIR)/$(CLIENT)

# Run server
run-server: $(SERVER)
	./$(SERVER)

# Run client
run-client: $(CLIENT)
	./$(CLIENT)

# Avoid conflicts with files named 'clean', 'all', etc.	
.PHONY: all clean run-server run-client