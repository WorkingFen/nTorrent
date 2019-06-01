#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include "client.hpp"
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <queue>
#include <iterator>
#include <fstream>
#include <algorithm>
#include <sys/stat.h>

class Client::FileManager
{
    char fileDirName[4096];
    std::string seedsDirName;
    std::string outputDirName;

    void removeFileIfFragmented(const std::string& fileName);           //  czyści katalog clientFiles
    int getNumberOfDownloadedBlocks(const std::string& fileName);
    int getDefaultNumberOfBlocks(const std::string& fileName);

    public:
    FileManager();
    ~FileManager();

    bool doesFileExist(const std::string& fileName, const bool isConfig);
    void printOutputFolderContent();
    void printSeedsFolderContent();
    std::vector<std::string> getDirFiles();   
    off_t getFileSize(const std::string& filename); 
    void putPiece(Client& client, const std::string& fileName, const int& index, const std::string& pieceData);           // metoda umieszczająca fragment pliku w pliku docelowym i aktualizująca plik konfiguracyjny
    void createConfig(Client& client, const std::string& fileName, const off_t& fileSize);                 // metoda tworząca plik konfiguracyjny (na jego początku docelowa liczba bloków) oraz plik docelowy, do którego będą umieszczane fragmenty
    void removeFragmentedFiles();
    std::vector<char> getBlockBytes(Client& client, const std::string& fileName, const int& index);
    bool doesBlockExist(Client& client, const std::string& fileName, const int& index);
    std::vector<int> getIndexesFromConfig(const std::string& fileName);
    void copyFile(const std::string& absoluteFilePath, const std::string& newFileName);
    bool isFileComplete(const std::string& fileName);
    void removeConfig(const std::string& fileName);
    void moveSeedToOutput(const std::string& fileName);

    std::string getSeedsDirName();
};

class FileManagerException : public std::exception
{
    const std::string info;

    public:
    FileManagerException(const std::string& msg);
    const char* what() const throw();
};

#endif