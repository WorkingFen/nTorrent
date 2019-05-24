#include "headers/fileManager.hpp"
#include <fstream>
FileManager::FileManager()
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

void FileManager::putPiece(std::string fileName, int index, int pieceLength, std::string pieceData) {      //Dla każdego pobieranego pliku tworzy plik.conf
    // chyba na starcie programu trzeba czyścić katalogi z niekompletnymi plikami i .conf

    //std::ofstream fileCreate(fileName.c_str()); // jeśli jeszcze nie istnieje, tworzy nowy plik
    //fileCreate.close();
	std::fstream filePieces(fileName.c_str());

	off_t offset = index * pieceLength;
	filePieces.seekp(long(offset), std::ios_base::beg);

	filePieces << pieceData;

	filePieces.close();

	std::string fileConfigName = fileName;
	fileConfigName += ".conf";

	std::ofstream fileConfig(fileConfigName.c_str(), std::ios::app);

	fileConfig << index;
	fileConfig << std::endl;

    fileConfig.close();
}