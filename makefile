CC = g++
CFLAGS = -std=c++11 -Wall -pthread

all: chat_client chat_server

chat_client: chat_client.cpp
	$(CC) $(CFLAGS) chat_client.cpp -o chat_client

chat_server: chat_server.cpp
	$(CC) $(CFLAGS) chat_server.cpp -o chat_server

clean:
	rm -f chat_client chat_server
