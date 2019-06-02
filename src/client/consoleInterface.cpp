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

void Client::ConsoleInterface::handleInputUp(Client &client, const std::vector<std::string> input)
{
    const std::string firstArg = input[0];

    if (firstArg == "help")
    {
        printHelp();
    }
    else if (firstArg == "connect")
    {
        client.connectTo(client.getServer(), SERVER);

        //wait for 210 before anything else

        state = State::connected;
    }
    else if (firstArg == "ls")
    {
        client.fileManager->printOutputFolderContent();
    }
    else if (firstArg == "disconnect" || firstArg == "file_list" || firstArg == "file_download" || firstArg == "seed_status")
    {
        cout << font.at("REDF") << "You are not connected to server yet." << font.at("RESETF") << endl;
    }
    else if (firstArg == "quit")
    {
        state = State::down;
    }
    else
    {
        cout << font.at("REDF") << "Nieprawidlowa komenda! Wpisz 'help', aby zobaczyc liste komend." << font.at("RESETF") << endl;
    }
}

void Client::ConsoleInterface::handleInputConnected(Client &client, const std::vector<std::string> input)
{
    const std::string firstArg = input[0];

    if (firstArg == "help")
    {
        printHelp();
    }
    else if (firstArg == "connect")
    {
        cout << font.at("REDF") << "You are already connected!" << font.at("RESETF") <<  endl;
    }
    else if (firstArg == "disconnect")
    {
        client.disconnect();
        state = State::up;
        messageState = MessageState::none;
        cout << font.at("SKYF") << "Disconnected!" << font.at("RESETF") << endl;
    }
    else if (firstArg == "ls")
    {
        client.fileManager->printOutputFolderContent();
    }
    else if (firstArg == "file_list")
    {
        client.listServerFiles();
    }
    else if (firstArg == "file_download")
    {
        fileDownload(client, input);
    }
    else if (firstArg == "stop_seeding" && (state == State::seeding || state == State::both))
    {
        fileDelete(client, input);
        std::cout << font.at("SKYF") << "File is no longer seeded!" << font.at("RESETF") << std::endl;
    }
    else if (firstArg == "file_add")
    {
        fileAdd(client, input);
        std::cout << font.at("SKYF") << "File adding complete!" << font.at("RESETF") << std::endl;
    }
    else if (firstArg == "seed_status")
    {
        client.fileManager->printSeedsFolderContent();
    }
    else if(firstArg == "state")
    {
        if(state == State::leeching)std::cout<< font.at("SKYF")<<"leeching"<< font.at("RESETF")<<std::endl;
        else if(state == State::seeding)std::cout<< font.at("SKYF")<<"seeding"<< font.at("RESETF")<<std::endl;
        else if(state == State::both)std::cout<< font.at("SKYF")<<"both"<< font.at("RESETF")<<std::endl;
        else if(state == State::connected)std::cout<< font.at("SKYF")<<"connected"<< font.at("RESETF")<<std::endl;
        else if(state == State::up)std::cout<< font.at("SKYF")<<"up"<< font.at("RESETF")<<std::endl;
        else if(state == State::down)std::cout<< font.at("SKYF")<<"down"<< font.at("RESETF")<<std::endl;
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
    cout << font.at("SKYF") << "------Lista komend:------" << endl
         << "[*] help                        - wypisz liste dostepnych komend" << endl
         << "[*] ls                          - pokaz zawartosc katalogu \"output\"" << endl;

    if (state == State::up)
    {
        cout << "[*] connect                     - polacz z serwerem" << endl;
    }
    if (state == State::connected || state == State::seeding ||
        state == State::leeching || state == State::both)
    {
        cout << "[*] disconnect                  - rozlacz sie z serwerem" << endl;
        cout << "[*] file_download <nazwa_pliku> - pobierz plik" << endl;
        cout << "[*] file_add <pelna_sciezka_pliku> <docelowa_nazwa_pliku>      - zacznij udostepniac plik (jesli chce sie udostepniac plik z katalogu \"output\" to wystarczy napisac \"./<nazwa_pliku>\"" << endl;
        cout << "[*] file_list                   - wylistuj pliki z serwera" << endl;
        cout << "[*] seed_status                 - pokaz zawartosc katalogu z plikami, ktore pobierasz/udostepniasz" << endl;
    }
    if (state == State::seeding || state == State::both)
    {
        cout << "[*] stop_seeding <nazwa_pliku>   - przestań udostępniać plik" << endl;
    }

    cout << "[*] quit                        - wylacz program" << font.at("RESETF") << endl;
}

void Client::ConsoleInterface::fileDownload(Client &client, const std::vector<std::string> &input)
{
    if (input.size() < 2)
    {
        printIncorrectCommand();
    }
    else
    {
        if(client.fileManager->doesFileExist(input[1], false))
            throw FileManagerException("Chosen file is already in your possession!");

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
        client.sendDeleteFile(input[1]); // wysłanie żądania o usunięcie pliku do serwera

        remove((client.fileManager->getSeedsDirName() + "/" + input[1]).c_str());  // usuń plik z folderu seeds

        if(client.fileManager->getDirFiles().size() == 0)        // jeśli w seeds jest pusto, to znaczy że już nic nie udostępniamy ani nie pobieramy  
        {
            if(state == State::both) state = State::connected;
            else if(state == State::seeding) state = State::connected;
        }
    }
}

void Client::ConsoleInterface::fileAdd(Client &client, const std::vector<std::string> &input)
{
    if (input.size() < 3)
    {
        printIncorrectCommand();
    }
    else
    {
        client.fileManager->copyFile(input[1], input[2]);
        client.shareFile(client.fileManager->getSeedsDirName(), input[2]); // wysłanie żądania o plik do serwera

        if(state==State::leeching) state = State::both;
        else if(state==State::connected) state = State::seeding;
    }
}

void Client::ConsoleInterface::printIncorrectCommand()
{
    cout << font.at("REDF") << "Nieprawidlowa komenda! Wpisz 'help', aby zobaczyc liste komend." << font.at("RESETF") << endl;
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
