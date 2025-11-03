CC = gcc
CFLAGS = -Wall -O2

SERVER = server
CLIENT = client

all: $(SERVER) $(CLIENT)

$(SERVER): server.c
	$(CC) $(CFLAGS) -o $(SERVER) server.c

$(CLIENT): client.c
	$(CC) $(CFLAGS) -o $(CLIENT) client.c

run_server: $(SERVER)
	./$(SERVER)

run_client: $(CLIENT)
	./$(CLIENT)

clean:
	rm -f $(SERVER) $(CLIENT)
