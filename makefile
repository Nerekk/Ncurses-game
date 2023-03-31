executable_server = server
executable_client = client

all: Client Server

Client: client.c game.c game.h
	gcc -g -Wall -pedantic -lncurses -lpthread client.c game.c -o $(executable_client)

Server: server.c game.c game.h
	gcc -g -Wall -pedantic -lncurses -lpthread server.c game.c -o $(executable_server)

.PHONY: clean
clean:
	rm $(executable_server)
	rm $(executable_client)