#ifndef CONSOLEINTERFACE_HPP
#define CONSOLEINTERFACE_HPP

#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

enum class State {down, up, connected};                       // stan, w jakim znajduje się użytkownik (determinuje obsługę i/o)

class ConsoleInterface
{
    char fileDirName[1000];
    DIR *fileDir;

    void printFolderContent();

    public:
    ConsoleInterface();
    ~ConsoleInterface();
    //void printMenu(State state);
    //void handleInput(State state, std::string input);
};

#endif