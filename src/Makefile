CC = g++
CPPFLAGS = -Wall -Wextra -pedantic -std=c++11

client:	client/headers/client.hpp client/client.cpp runClient.cpp
	$(CC) -o run client/headers/client.hpp client/client.cpp runClient.cpp $(CPPFLAGS)

all: client