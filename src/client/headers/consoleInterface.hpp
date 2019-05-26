#ifndef CONSOLEINTERFACE_HPP
#define CONSOLEINTERFACE_HPP

#include "client.hpp"
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <queue>

enum class State      // stan, w jakim znajduje się użytkownik (determinuje obsługę i/o)
{
    up,               // niepołączony z serwerem  
    connected,        // połączony z serwerem, nic nie udostępnia ani nie pobiera
    seeding,          // udostępnia plik(i), nic nie pobiera
    leeching,         // pobiera plik(i), nic nie udostępnia
    both,             // jednocześnie udostępnia i pobiera
    down,             // użytkownik wybrał opcję zakończenia programu
};

enum class MessageState
{
    wait_for_file_info,    // oczekiwanie na odpowiedź od serwera na zapytanie o plik
    wait_for_block_info,   // oczekiwanie na odpowiedź od serwera na zapytanie o plik
    none
};

class Client::ConsoleInterface
{
    State state;
    MessageState messageState;
    std::string chosenFile;
    std::vector<char> buffer;
    std::queue<std::string> commandQueue;

    void handleInputUp(Client& client, std::vector<std::string> input);
    void handleInputConnected(Client& client, std::vector<std::string> input);

    void printHelp();
    void fileDownload(Client &client, const std::vector<std::string> &input);
    void fileDelete(Client &client, const std::vector<std::string> &input);
    void fileAdd(Client &client, const std::vector<std::string> &input);

    public:
    ConsoleInterface();
    ~ConsoleInterface();

    void processCommands(const char* buf);
    std::vector<std::string> splitBySpace(const std::string &input);       // pewnie powinno być w jakimś utils.h
    void handleInput(Client& client);
    void printIncorrectCommand();

    void stopSeeding();
    void stopLeeching();

    State getState();
    MessageState getMessageState();
    void setMessageState(MessageState state);

    std::string getChosenFile();
};

#endif