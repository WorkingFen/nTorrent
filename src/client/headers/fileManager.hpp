#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include "client.hpp"
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <queue>
#include <sys/stat.h>

class Client;

class FileManager
{
    Client& client;
    char fileDirName[1000];
    DIR *fileDir;

    public:
    FileManager(Client& c);
    ~FileManager();

    void printFolderContent();
    void setDir();
    std::vector<std::string> getDirFiles();   
    off_t getFileSize(const std::string& filename); 
};

#endif