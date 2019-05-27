CC = g++
CPPFLAGS = -Wall -Wextra -pedantic -std=c++14 -pthread

client: client/headers/client.hpp client/client.cpp runClient.cpp message/message.cpp message/message.hpp client/headers/consoleInterface.hpp hashing/headers/hash.h hashing/headers/sha256.h
	$(CC) -o runClient client/headers/client.hpp client/client.cpp runClient.cpp message/message.cpp message/message.hpp client/headers/consoleInterface.hpp client/consoleInterface.cpp client/headers/fileManager.hpp client/fileManager.cpp hashing/hash.cpp hashing/sha256.cpp $(CPPFLAGS)
	mkdir -p -m 777 $(PWD)/clientFiles
	mkdir -p -m 777 $(PWD)/clientFiles/configFiles

server: server/headers/server.hpp server/server.cpp runServer.cpp message/message.cpp message/message.hpp
	$(CC) -o runServer server/headers/server.hpp server/server.cpp runServer.cpp message/message.cpp message/message.hpp $(CPPFLAGS)

all: client server
	mkdir -p bin
	mkdir -p bin/client1
	mkdir -p bin/client1/clientFiles
	mkdir -p bin/client2
	mkdir -p bin/client2/clientFiles
	cp runClient bin/client1
	cp runClient bin/client2
	cp runServer bin

ctclean:
	rm -r runClient $(PWD)/clientFiles

srvclean:
	rm -r runServer

clean:
	rm -r runClient runServer $(PWD)/clientFiles