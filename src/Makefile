CC = g++
CPPFLAGS = -Wall -Wextra -pedantic -std=c++14 -pthread

client: client/headers/client.hpp client/client.cpp runClient.cpp message/message.cpp message/message.hpp client/headers/consoleInterface.hpp hashing/headers/hash.h hashing/headers/sha256.h
	$(CC) -o runClient client/headers/client.hpp client/client.cpp runClient.cpp message/message.cpp message/message.hpp client/headers/consoleInterface.hpp client/consoleInterface.cpp client/headers/fileManager.hpp client/fileManager.cpp hashing/hash.cpp hashing/sha256.cpp $(CPPFLAGS)
	mkdir -p -m 777 $(PWD)/seeds
	mkdir -p -m 777 $(PWD)/seeds/configFiles
	mkdir -p -m 777 $(PWD)/output

server: server/headers/server.hpp server/server.cpp runServer.cpp message/message.cpp message/message.hpp
	$(CC) -o runServer server/headers/server.hpp server/server.cpp runServer.cpp message/message.cpp message/message.hpp $(CPPFLAGS)

all: client server
	mkdir -p bin
	mkdir -p bin/client1
	mkdir -p bin/client1/output
	mkdir -p bin/client1/seeds
	mkdir -p bin/client1/seeds/configFiles
	mkdir -p bin/client2
	mkdir -p bin/client2/output
	mkdir -p bin/client2/seeds
	mkdir -p bin/client2/seeds/configFiles
	cp runClient bin/client1
	cp runClient bin/client2
	cp runServer bin

ctclean:
	rm -r runClient $(PWD)/seeds
	rm -r runClient $(PWD)/output

srvclean:
	rm -r runServer

clean:
	rm -r runClient runServer $(PWD)/seeds $(PWD)/output