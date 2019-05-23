#include "headers/consoleInterface.hpp"
#include "../hashing/headers/hash.h"
using std::cout;
using std::endl;
using std::string;
using std::vector;


ConsoleInterface::ConsoleInterface(Client& c): client(c), state(State::up)
{
    getcwd(fileDirName, 1000);
    strcat(fileDirName, "/clientFiles");
    fileDir = opendir(fileDirName);

    if(fileDir == NULL)
        throw std::runtime_error ("opendir call failed");
}

ConsoleInterface::~ConsoleInterface()
{
    closedir(fileDir);
}

void ConsoleInterface::printFolderContent()
{
    struct dirent *x;
    int i=1;
    cout << endl << "ZAWARTOSC KATALOGU:" << endl; 
    while( (x=readdir(fileDir)) != NULL )
    {
        if(strcmp(x->d_name, ".")!=0 && strcmp(x->d_name, "..")!=0)
        {
            cout << i << ". " << x->d_name << endl;
            i++;
        }
    }
    cout << endl;
    rewinddir(fileDir);
}

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
        //calculateHashes();
        client.shareFiles();
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
        printFolderContent();
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

vector<string> ConsoleInterface::getDirFiles(){
    struct dirent *x;
    vector<string> fileNames;
    while( (x=readdir(fileDir)) != NULL )
    {
        if(strcmp(x->d_name, ".")!=0 && strcmp(x->d_name, "..")!=0)
        {
            fileNames.push_back(x->d_name);
        }
    }
    rewinddir(fileDir);
    return fileNames;
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

off_t ConsoleInterface::getFileSize(const std::string& filename)
{
    struct stat fileStats;
    std::string filePath(fileDirName);
    filePath = filePath + "/" + filename;

    if(stat(filePath.c_str(), &fileStats) == -1)
        throw std::runtime_error("Error while retrieving info about file!");
    
    return fileStats.st_size;
}