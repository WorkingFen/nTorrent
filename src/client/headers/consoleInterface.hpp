#ifndef CONSOLEINTERFACE_HPP
#define CONSOLEINTERFACE_HPP

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
    both              // jednocześnie udostępnia i pobiera
};


class ConsoleInterface
{
    char fileDirName[1000];
    DIR *fileDir;

    void printFolderContent();

    public:
    ConsoleInterface();
    ~ConsoleInterface();

    void setDir();
    std::vector<std::string> getDirFiles();
    void calculateHashes();
    void printMenu(State state);
    void handleInput(State state, int input);
};

#endif