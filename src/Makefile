CC = g++
CPPFLAGS = -Wall -Wextra -pedantic -std=c++11 -pthread

client: client/headers/client.hpp client/client.cpp runClient.cpp client/headers/message.hpp
	$(CC) -o runClient client/headers/client.hpp client/client.cpp runClient.cpp client/headers/message.hpp $(CPPFLAGS)
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