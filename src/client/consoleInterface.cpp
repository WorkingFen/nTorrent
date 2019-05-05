#include "headers/consoleInterface.hpp"

void ConsoleInterface::printFolderContent()
{
    struct dirent *x;
    while( (x=readdir(fileDir)) != NULL )
    {
        if(!strcmp(x->d_name, ".") && !strcmp(x->d_name, ".."))
            std::cout << x->d_name << std::endl;
    }
}

ConsoleInterface::ConsoleInterface()
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