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

class Client::ConsoleInterface
{
    State state;
    std::vector<char> buffer;
    std::queue<std::string> commandQueue;

    void handleInputUp(Client& client, std::vector<std::string> input);
    void handleInputConnected(Client& client, std::vector<std::string> input);

    public:
    ConsoleInterface();
    ~ConsoleInterface();

    void processCommands(const char* buf);
    std::vector<std::string> splitBySpace(std::string input);       // pewnie powinno być w jakimś utils.h
    void handleInput(Client& client);

    void stopSeeding();
    void stopLeeching();
    State getState();
};

#endif