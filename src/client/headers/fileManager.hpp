#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <queue>
#include <sys/stat.h>

class FileManager
{
    char fileDirName[1000];
    DIR *fileDir;

    public:
    FileManager();
    ~FileManager();

    void printFolderContent();
    void setDir();
    std::vector<std::string> getDirFiles();   
    off_t getFileSize(const std::string& filename); 
    void putPiece(std::string fileName, int index, int pieceLength, std::string pieceData);        

};

#endif