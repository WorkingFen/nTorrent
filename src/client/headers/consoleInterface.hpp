#ifndef CONSOLEINTERFACE_HPP
#define CONSOLEINTERFACE_HPP

#include "client.hpp"
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <vector>

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
    char fileDirName[1000];
    DIR *fileDir;
    State state;

    void printFolderContent();

    public:
    ConsoleInterface(Client& c);
    ~ConsoleInterface();

    void setDir();
    std::vector<std::string> getDirFiles();
    void calculateHashes();
    void printMenu();
    void printConnected();
    void handleInput(int input);
    void handleInputUp(int input);
    void handleInputConnected(int connected);

    void stopSeeding();
    void stopLeeching();
    State getState();
};

#endif