CC = g++
CPPFLAGS = -Wall -Wextra -pedantic -std=c++11

client:	client/headers/client.hpp client/client.cpp runClient.cpp
	$(CC) -o runClient client/headers/client.hpp client/client.cpp runClient.cpp $(CPPFLAGS)

server: server/headers/server.hpp server/server.cpp runServer.cpp
	$(CC) -o runServer server/headers/server.hpp server/server.cpp runServer.cpp $(CPPFLAGS)

all: client server
	mkdir -p -m 777 $(PWD)/clientFiles

clntclean:
	rm -r runClient

srvclean:
	rm -r runServer

clean:
	rm -r clientFiles