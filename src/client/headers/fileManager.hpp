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

class Client::FileManager
{
    char fileDirName[1000];

    void removeFileIfFragmented(const std::string& fileName);

    public:
    FileManager();
    ~FileManager();

    void printFolderContent();
    void setDir();
    std::vector<std::string> getDirFiles();   
    off_t getFileSize(const std::string& filename); 
    void putPiece(Client& client, const std::string& fileName, const int& index, const std::string& pieceData);           // metoda umieszczająca fragment pliku w pliku docelowym i aktualizująca plik konfiguracyjny
    void createConfig(Client& client, const std::string& fileName, const off_t& fileSize);                 // metoda tworząca plik konfiguracyjny (na jego początku docelowa liczba bloków) oraz plik docelowy, do którego będą umieszczane fragmenty
    void removeFragmentedFiles();
    std::vector<char> getBlockBytes(Client& client, const std::string& fileName, const int& index);  
    std::vector<int> getIndexesFromConfig(const std::string& fileName);                 // metoda zwracające indeksy fragmentów znajdujących się w pliku docelowym
};

#endif