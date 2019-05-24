#include "headers/consoleInterface.hpp"
#include "../hashing/headers/hash.h"
using std::cout;
using std::endl;
using std::string;
using std::vector;


ConsoleInterface::ConsoleInterface(Client& c): client(c), state(State::up) {}

ConsoleInterface::~ConsoleInterface() {}

void ConsoleInterface::handleInputUp(){
    std::string input=commandQueue.front();
    commandQueue.pop();
    if(input == "help")
    {
        cout << "Lista komend:" << endl
        << "help - wypisz liste dostepnych komend" << endl
        << "connect - polacz z serwerem" << endl
        << "ls - pokaz zawartosc katalogu z plikami, ktore udostepniasz (wymagane polaczenie z serwerem)" << endl
        << "disconnect - rozlacz sie z serwerem" << endl
        << "quit - wylacz program" << endl;
    }   else if(input == "connect")
    {
        client.connectTo(client.getServer());

        //wait for 210 before anything else

        state = State::connected;
    }   else if (input == "quit")
    {
        state = State::down;
    }   else
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
void ConsoleInterface::handleInputConnected(){
    std::string input=commandQueue.front();
    commandQueue.pop();
    if(input == "help")
    {
        cout << "Lista komend:" << endl
        << "help - wypisz liste dostepnych komend" << endl
        << "connect - polacz z serwerem" << endl
        << "ls - pokaz zawartosc katalogu z plikami, ktore udostepniasz (wymagane polaczenie z serwerem)" << endl
        << "disconnect - rozlacz sie z serwerem" << endl
        << "quit - wylacz program" << endl;
    }   else if(input == "connect")
    {
        cout << "Jestes juz polaczony!" << endl;
    }   else if(input == "ls")
    {
        client.printFolderContent();
    }   else if (input == "quit")
    {
        state = State::down;
    }   else
    {
        cout << "Nieprawidlowa komenda! Wpisz 'help', aby zobaczyc liste komend." << endl;
    }
/*
    switch(input){
        case 1:
                printFolderContent();
                break;
        
        case 2:
                //TODO
                break;
        case 3:
                break;

        case 4:
                if(state == State::seeding) {stopSeeding();}
                if(state == State::leeching || state == State::both) {stopLeeching();}
                break;

        case 5:
                if(state == State::both) {stopSeeding();}
                break;
                
        case 9:
                state = State::down;
                break;

        default:
                //TODO
                break;

    }
*/
}

void ConsoleInterface::processCommands(const char* buf)
{
    buffer.insert(buffer.end(), buf, buf+strlen(buf));
    for(auto it = buffer.begin(); it!=buffer.end();)
    {
        if(*it==10)
        {
            commandQueue.emplace(std::string(buffer.begin(), it));
            it=buffer.erase(buffer.begin(), it+1);
        }
        else
        {
            ++it;
        } 
    }
}

void ConsoleInterface::handleInput()
{
    if(!commandQueue.empty())
    {
        if(state == State::up)  handleInputUp();
        else if(state == State::connected || state == State::seeding || state == State::seeding || state == State::both) handleInputConnected(); // prawie takie same
    }
}

void ConsoleInterface::stopSeeding()
{
    //TODO
}

void ConsoleInterface::stopLeeching()
{
    //TODO
}

State ConsoleInterface::getState(){
    return state;
}
