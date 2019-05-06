CC = g++
CPPFLAGS = -Wall -Wextra -pedantic -std=c++14 -pthread

client: client/headers/client.hpp client/client.cpp runClient.cpp client/headers/message.hpp client/headers/consoleInterface.hpp hashing/headers/hash.h hashing/headers/sha256.h
	$(CC) -o runClient client/headers/client.hpp client/client.cpp runClient.cpp client/headers/message.hpp client/headers/consoleInterface.hpp client/consoleInterface.cpp hashing/hash.cpp hashing/sha256.cpp $(CPPFLAGS)
	mkdir -p -m 777 $(PWD)/clientFiles

server: server/headers/server.hpp server/server.cpp runServer.cpp client/headers/message.hpp
	$(CC) -o runServer server/headers/server.hpp server/server.cpp runServer.cpp client/headers/message.hpp $(CPPFLAGS)


all: client server

ctclean:
	rm -r runClient $(PWD)/clientFiles

srvclean:
	rm -r runServer

clean:
	rm -r runClient runServer $(PWD)/clientFiles