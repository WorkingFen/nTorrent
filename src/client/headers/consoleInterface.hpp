#ifndef CONSOLEINTERFACE_HPP
#define CONSOLEINTERFACE_HPP

#include "client.hpp"
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <queue>
#include <sys/stat.h>

enum class State      // stan, w jakim znajduje się użytkownik (determinuje obsługę i/o)
{
    up,               // niepołączony z serwerem  
    connected,        // połączony z serwerem, nic nie udostępnia ani nie pobiera
    seeding,          // udostępnia plik(i), nic nie pobiera
    leeching,         // pobiera plik(i), nic nie udostępnia
    both,             // jednocześnie udostępnia i pobiera
    down              // użytkownik wybrał opcję zakończenia program
};

class Client;

class ConsoleInterface
{
    Client& client;
    State state;
    std::vector<char> buffer;
    std::queue<std::string> commandQueue;

    void handleInputUp();
    void handleInputConnected();

    public:
    ConsoleInterface(Client& c);
    ~ConsoleInterface();

    void processCommands(const char* buf);
    void handleInput();

    void stopSeeding();
    void stopLeeching();
    State getState();
};

#endif