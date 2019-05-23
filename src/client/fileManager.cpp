#include "headers/fileManager.hpp"

FileManager::FileManager(Client& c) : client(c)
{
    getcwd(fileDirName, 1000);
    strcat(fileDirName, "/clientFiles");
    fileDir = opendir(fileDirName);

    if(fileDir == NULL)
        throw std::runtime_error ("opendir call failed");
}

FileManager::~FileManager()
{
    closedir(fileDir);
}

void FileManager::printFolderContent()
{
    struct dirent *x;
    int i=1;
    std::cout << std::endl << "ZAWARTOSC KATALOGU:" << std::endl; 
    while( (x=readdir(fileDir)) != NULL )
    {
        if(strcmp(x->d_name, ".")!=0 && strcmp(x->d_name, "..")!=0)
        {
            std::cout << i << ". " << x->d_name << std::endl;
            i++;
        }
    }
    std::cout << std::endl;
    rewinddir(fileDir);
}

void FileManager::setDir()
{

}

std::vector<std::string> FileManager::getDirFiles()
{
    struct dirent *x;
    std::vector<std::string> fileNames;
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

off_t FileManager::getFileSize(const std::string& filename)
{
    struct stat fileStats;
    std::string filePath(fileDirName);
    filePath = filePath + "/" + filename;

    if(stat(filePath.c_str(), &fileStats) == -1)
        throw std::runtime_error("Error while retrieving info about file!");
    
    return fileStats.st_size;
}