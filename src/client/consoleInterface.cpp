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

void ConsoleInterface::printMenu()
{
    switch(state){
        case State::up:
            cout<<"Menu:"<<endl
                <<"Aktualny stan: niepołączony"<<endl
                <<"1. Połącz z serwerem"<<endl
                <<"9. Zakończ program"<<endl;
            break;
        case State::connected:
            cout<<"Menu:"<<endl
                <<"Aktualny stan: połączony"<<endl
                <<"1. Wyświetl zawartość katalogu"<<endl
                <<"2. Pobierz plik"<<endl
                <<"3. Uaktualnij stan plików"<<endl
                <<"9. Zakończ program"<<endl;
            break;
        case State::seeding:
            cout<<"Menu:"<<endl
                <<"Aktualny stan: udostępnianie"<<endl
                <<"1. Przestań udostępniać"<<endl
                <<"2. Zakończ program"<<endl
                <<"3. Zakończ program"<<endl
                <<"4. Zakończ program"<<endl;
            break;
        case State::leeching:
            cout<<"Menu:"<<endl
                <<"Aktualny stan: pobieranie"<<endl
                <<"1. Połącz z serwerem"<<endl
                <<"2. Zakończ program"<<endl;
            break;
        case State::both:
            cout<<"Menu:"<<endl
                <<"Aktualny stan: udostępnianie/pobieranie"<<endl
                <<"1. Połącz z serwerem"<<endl
                <<"2. Zakończ program"<<endl;
            break;
        case State::down:
            return;

        default:
            cout<<"Menu error"<<endl;
            return;
    }
    cout<<"Wprowadź numer opcji: ";

}

void ConsoleInterface::handleInput(int input){
    //std::system("clear");
    switch(state){
        case State::up:
            handleInputUp(input);
            break;
        case State::connected:
            handleInputConnected(input);
            break;
        case State::seeding:
            handleInputSeeding(input);
            break;
        case State::leeching:
            handleInputLeeching(input);
            break;
        case State::both:
            handleInputBoth(input);
            break;
        default:
            //TODO - przy State::down chyba nie powinno się nic dziać, pętla zakończy działanie po dokończeniu obiegu
            break;
    }
}

void ConsoleInterface::handleInputUp(int input){
    switch(input){
        case 1:
                calculateHashes();
                client.connectTo(client.getServer());
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

void ConsoleInterface::handleInputConnected(int input){
    switch(input){
        case 1:
                printFolderContent();
                break;
                
        case 9:
                state = State::down;
                break;

        default:
                //TODO
                break;

    }
}

void ConsoleInterface::handleInputSeeding(int input){
    switch(input){
        case 1:
                printFolderContent();
                break;
                
        case 9:
                state = State::down;
                break;

        default:
                //TODO
                break;

    }
}

void ConsoleInterface::handleInputLeeching(int input){
    switch(input){
        case 1:
                printFolderContent();
                break;
                
        case 9:
                state = State::down;
                break;

        default:
                //TODO
                break;

    }
}

void ConsoleInterface::handleInputBoth(int input){
    switch(input){
        case 1:
                printFolderContent();
                break;
                
        case 9:
                state = State::down;
                break;

        default:
                //TODO
                break;

    }
}

State ConsoleInterface::getState(){
    return state;
}

void ConsoleInterface::printFolderContent()
{
    struct dirent *x;
    while( (x=readdir(fileDir)) != NULL )
    {
        if(strcmp(x->d_name, ".")!=0 && strcmp(x->d_name, "..")!=0)
            cout << x->d_name << endl;
    }
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