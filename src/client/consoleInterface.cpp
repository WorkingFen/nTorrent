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

void ConsoleInterface::printConnected()
{
    cout<<"Menu:"<<endl
        <<"Aktualny stan: połączony"<<endl
        <<"1. Wyświetl zawartość katalogu"<<endl
        <<"2. Pobierz plik"<<endl
        <<"3. Uaktualnij stan plików"<<endl;
}

void ConsoleInterface::printMenu()
{
    switch(state){
        case State::up:
            cout<<"Menu:"<<endl
                <<"Aktualny stan: niepołączony"<<endl
                <<"1. Połącz z serwerem"<<endl;
            break;

        case State::connected:
            printConnected();
            break;

        case State::seeding:
            printConnected();
            cout<<"4. Zakończ udostępniać plik"<<endl;
            break;

        case State::leeching:
            printConnected();
            cout<<"4. Zakończ pobierać plik"<<endl;
            break;

        case State::both:
            printConnected();
            cout<<"4. Zakończ pobierać plik"<<endl
                <<"5. Zakończ udostępniać plik"<<endl;
            break;

        case State::down:
            return;

        default:
            cout<<"Menu error"<<endl;
            return;
    }

    cout<<"9. Zakończ program"<<endl;
    cout<<"Wprowadź numer opcji: ";

}

void ConsoleInterface::handleInput(int input){
    //std::system("clear");
    
    if(state == State::up)  handleInputUp(input);
    else if(state == State::connected || state == State::seeding || state == State::seeding || state == State::both) handleInputConnected(input); // prawie takie same
    else return;

}

void ConsoleInterface::handleInputUp(int input){
    switch(input){
        case 1:
                client.connectTo(client.getServer());
                calculateHashes();
                state = State::connected;
                break;
                
        case 9:
                state = State::down;
                break;

        default:
                //TODO
                break;

    }
}
/*
    1. Wyświetl zawartość katalogu
    2. Pobierz plik
    3. Uaktualnij stan plików
    4. Zakończ pobierać plik
    5. Zakończ udostępniać plik

*/
void ConsoleInterface::handleInputConnected(int input){
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

vector<string> ConsoleInterface::getDirFiles(){
    struct dirent *x;
    vector<string> fileNames;
    while( (x=readdir(fileDir)) != NULL )
    {
        if(strcmp(x->d_name, ".")!=0 && strcmp(x->d_name, "..")!=0)
        {
            cout << "FILENAME="<<x->d_name << endl;
            fileNames.push_back(x->d_name);
        }
    }
    rewinddir(fileDir);
    return fileNames;
}

void ConsoleInterface::calculateHashes(){
    vector<string> fileNames = getDirFiles();
    vector<vector<string>> hashes;
    string path;
    cout<<fileDirName<<endl;
    for(auto it = fileNames.begin(); it != fileNames.end(); ++it){

        cout<<fileDirName<<":"<<*it<<endl;
        path = fileDirName + *it;

        hashes.push_back(hashPieces(path, 10));
    }
}