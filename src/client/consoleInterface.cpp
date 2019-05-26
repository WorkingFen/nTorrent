#include "headers/consoleInterface.hpp"
#include "headers/fileManager.hpp"
#include "../hashing/headers/hash.h"
#include <sstream>
using std::cout;
using std::endl;
using std::string;
using std::vector;

Client::ConsoleInterface::ConsoleInterface() : state(State::up), messageState(MessageState::none) {}

Client::ConsoleInterface::~ConsoleInterface() {}

void Client::ConsoleInterface::handleInputUp(Client &client, std::vector<std::string> input)
{
    std::string firstArg = input[0];

    if (firstArg == "help")
    {
        printHelp();
    }
    else if (firstArg == "connect")
    {
        client.connectTo(client.getServer());

        //wait for 210 before anything else

        state = State::connected;
    }
    else if (firstArg == "ls")
    {
        client.fileManager->printFolderContent();
    }
    else if (firstArg == "disconnect")
    {
        cout << "Nie jestes polaczony z serwerem." << endl;
    }
    else if (firstArg == "quit")
    {
        state = State::down;
    }
    else
    {
        cout << "Nieprawidlowa komenda! Wpisz 'help', aby zobaczyc liste komend." << endl;
    }
}
/*
    1. Wyświetl zawartość katalogu
    2. Pobierz plik
    3. Uaktualnij stan plików
    4. Zakończ pobierać plik
    5. Zakończ udostępniać plik

*/
void Client::ConsoleInterface::handleInputConnected(Client &client, std::vector<std::string> input)
{
    std::string firstArg = input[0];

    if (firstArg == "help")
    {
        printHelp();
    }
    else if (firstArg == "connect")
    {
        cout << "Jestes juz polaczony!" << endl;
    }
    else if (firstArg == "disconnect")
    {
        client.disconnect();
        state = State::up;
        messageState = MessageState::none;
    }
    else if (firstArg == "ls")
    {
        client.fileManager->printFolderContent();
    }
    else if (firstArg == "file_download")
    {
        fileDownload(client, input);
    }
    else if (firstArg == "file_delete" && (state == State::seeding || state == State::both))
    {
        fileDelete(client, input);
    }
    else if (firstArg == "file_add")
    {
        fileAdd(client, input);
    }
    else if (firstArg == "quit")
    {
        state = State::down;
    }
    else
    {
        printIncorrectCommand();
    }
}

void Client::ConsoleInterface::printHelp()
{
    cout << "Lista komend:" << endl
         << "help                        - wypisz liste dostepnych komend" << endl
         << "ls                          - pokaz zawartosc katalogu z plikami, ktore udostepniasz" << endl;

    if (state == State::up)
    {
        cout << "connect                     - polacz z serwerem" << endl;
    }
    if (state == State::connected || state == State::seeding ||
        state == State::leeching || state == State::both)
    {
        cout << "disconnect                  - rozlacz sie z serwerem" << endl;
        cout << "file_download <nazwa_pliku> - pobierz plik" << endl;
        cout << "file_add <nazwa_pliku>      - zacznij udostępniać plik" << endl;
    }
    if (state == State::seeding || state == State::both)
    {
        cout << "file_delete <nazwa_pliku>   - przestań udostępniać plik" << endl;
    }

    cout << "quit                        - wylacz program" << endl;
}

void Client::ConsoleInterface::fileDownload(Client &client, const std::vector<std::string> &input)
{
    if (input.size() < 2)
    {
        printIncorrectCommand();
    }
    else
    {
        client.sendAskForFile(input[1]);                 // wysłanie żądania o plik do serwera
        messageState = MessageState::wait_for_file_info; // ustawienie stanu na czekanie na odpowiedź od serwera o pliku
        chosenFile = input[1];                           // zapisanie wybranego pliku
    
        if(state == State::seeding) state = State::both;
        if(state == State::connected) state = State::leeching;
    }
}

void Client::ConsoleInterface::fileDelete(Client &client, const std::vector<std::string> &input)
{
    if (input.size() < 2)
    {
        printIncorrectCommand();
    }
    else
    {
        client.sendDeleteFile(input[1]); // wysłanie żądania o plik do serwera
        // + zakończenie komunikacji z klientem?

        // + ewentualna zmiana stanu: both->leeching lub seeding->connected, 
        //   jeśli był to jedyny udostępniany plik 
    }
}

void Client::ConsoleInterface::fileAdd(Client &client, const std::vector<std::string> &input)
{
    if (input.size() < 2)
    {
        printIncorrectCommand();
    }
    else
    {
        client.shareFile("clientFiles", input[1]); // wysłanie żądania o plik do serwera

        if(state==State::leeching) state = State::both;
        else if(state==State::connected) state = State::seeding;
    }
}

void Client::ConsoleInterface::printIncorrectCommand()
{
    cout << "Nieprawidlowa komenda! Wpisz 'help', aby zobaczyc liste komend." << endl;
}

void Client::ConsoleInterface::processCommands(const char *buf)
{
    buffer.insert(buffer.end(), buf, buf + strlen(buf));
    for (auto it = buffer.begin(); it != buffer.end();)
    {
        if (*it == 10)
        {
            commandQueue.emplace(std::string(buffer.begin(), it));
            it = buffer.erase(buffer.begin(), it + 1);
        }
        else
        {
            ++it;
        }
    }
}

std::vector<std::string> Client::ConsoleInterface::splitBySpace(const std::string &input)
{
    std::string buf;
    std::stringstream ss(input);

    std::vector<std::string> tokens;

    while (ss >> buf)
        tokens.push_back(buf);

    return tokens;
}

void Client::ConsoleInterface::handleInput(Client &client)
{
    if (!commandQueue.empty())
    {
        std::string input = commandQueue.front();
        commandQueue.pop();
        vector<std::string> tokens = splitBySpace(input);
        if (tokens.size() == 0)
            tokens.push_back("");

        if (state == State::up)
            handleInputUp(client, tokens);

        else if (state == State::connected || state == State::seeding || state == State::leeching || state == State::both)
            handleInputConnected(client, tokens); // prawie takie same
    }
}

void Client::ConsoleInterface::stopSeeding()
{
    //TODO
}

void Client::ConsoleInterface::stopLeeching()
{
    //TODO
}

State Client::ConsoleInterface::getState()
{
    return state;
}

void Client::ConsoleInterface::setState(State newState)
{
    state = newState;
}

MessageState Client::ConsoleInterface::getMessageState()
{
    return messageState;
}

void Client::ConsoleInterface::setMessageState(MessageState newState)
{
    messageState = newState;
}

std::string Client::ConsoleInterface::getChosenFile()
{
    return chosenFile;
}
