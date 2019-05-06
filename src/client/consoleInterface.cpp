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
    cout<<"Menu:"<<endl;
    switch(state){
        case State::up:
            cout<<"Aktualny stan: niepołączony"<<endl
                <<"1. Połącz z serwerem"<<endl
                <<"9. Zakończ program"<<endl;
            break;
        case State::connected:
            cout<<"Aktualny stan: połączony"<<endl
                <<"1. Wyświetl zawartość katalogu"<<endl
                <<"2. Pobierz plik"<<endl
                <<"3. Uaktualnij stan plików"<<endl
                <<"9. Zakończ program"<<endl;
            break;
        case State::seeding:
            cout<<"Aktualny stan: udostępnianie"<<endl
                <<"1. Przestań udostępniać"<<endl
                <<"2. Zakończ program"<<endl
                <<"3. Zakończ program"<<endl
                <<"4. Zakończ program"<<endl;
            break;
        case State::leeching:
            cout<<"Aktualny stan: pobieranie"<<endl
                <<"1. Połącz z serwerem"<<endl
                <<"2. Zakończ program"<<endl;
            break;
        case State::both:
            cout<<"Aktualny stan: udostępnianie/pobieranie"<<endl
                <<"1. Połącz z serwerem"<<endl
                <<"2. Zakończ program"<<endl;
            break;

        default:
            cout<<"Error"<<endl;
            break;
    }
    cout<<"Wprowadź numer opcji: ";

}

void ConsoleInterface::handleInput(int input){
    //std::system("clear");
    switch(state){
        case State::up:
            if(input == 1)
            {
                calculateHashes();
                client.connectTo(client.getServer());
                state = State::connected;
            }
            break;
        case State::connected:
            if(input == 1)
            {
                printFolderContent();
            }
            break;
        case State::seeding:
            //TODO
            break;
        case State::leeching:
            //TODO
            break;
        case State::both:
            //TODO
            break;
        default:
            //TODO
            break;
    }
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