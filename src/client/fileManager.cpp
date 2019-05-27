#include "headers/fileManager.hpp"

#include <iostream>

Client::FileManager::FileManager()
{
    getcwd(fileDirName, 1000);
    strcat(fileDirName, "/clientFiles");
}

Client::FileManager::~FileManager() {}

void Client::FileManager::removeFileIfFragmented(const std::string& fileName)
{
    std::string path(fileDirName);
    path = path + "/" + fileName;

    std::ifstream inFile;
    inFile.open(path + ".conf");

    if(!inFile.is_open())
        throw FileManagerException ("File: " + path + ".conf could not be opened.");

    int downloadedBlocksNumber = std::count(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>(), '\n') - 1;
    int defaultBlocksNumber;
    inFile.seekg(0);
    inFile >> defaultBlocksNumber;
    
    if(downloadedBlocksNumber < defaultBlocksNumber)
    {
        remove((path + ".conf").c_str());
        remove(path.c_str());
    }
}

void Client::FileManager::printFolderContent()
{
    DIR *fileDir = opendir(fileDirName);
    
    if(fileDir == NULL)
        throw FileManagerException ("opendir call failed");

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
    closedir(fileDir);
}

void Client::FileManager::setDir()
{

}

std::vector<std::string> Client::FileManager::getDirFiles()
{
    DIR *fileDir = opendir(fileDirName);
    if(fileDir == NULL)
        throw FileManagerException ("opendir call failed");  

    struct dirent *x;
    std::vector<std::string> fileNames;
    while( (x=readdir(fileDir)) != NULL )
    {
        if(strcmp(x->d_name, ".")!=0 && strcmp(x->d_name, "..")!=0)
        {
            fileNames.push_back(x->d_name);
        }
    }
    closedir(fileDir);
    return fileNames;
} 

off_t Client::FileManager::getFileSize(const std::string& filename)
{
    struct stat fileStats;
    std::string filePath(fileDirName);
    filePath = filePath + "/" + filename;

    if(stat(filePath.c_str(), &fileStats) == -1)
        throw FileManagerException("Error while retrieving info about file!");
    
    return fileStats.st_size;
}

void Client::FileManager::putPiece(Client& client, const std::string& fileName, const int& index, const std::string& pieceData) 
{
    std::string path(fileDirName);
    path = path + "/" + fileName;

	std::fstream filePieces;
    filePieces.open(path);

    if(!filePieces.is_open())
        throw FileManagerException ("File: " + path + " could not be opened.");

	off_t offset = index * client.pieceSize;
	filePieces.seekp(long(offset), std::ios_base::beg);

	filePieces << pieceData;

	filePieces.close();

	std::ofstream configFile;
    configFile.open(path + ".conf", std::ios::app);

    if(!configFile.is_open())
        throw FileManagerException ("File: " + path + ".conf could not be opened.");

	configFile << index;
	configFile << std::endl;

    configFile.close();
}

void Client::FileManager::createConfig(Client& client, const std::string& fileName, const off_t& fileSize)
{
    std::string path(fileDirName);
    path = path + "/" + fileName;

    std::ofstream newFile(path, std::ios::ate);             // tworzy nowy plik

    newFile.seekp(fileSize - 1);
    newFile.write("",1);
    newFile.seekp(0);
    newFile.close();

    std::ofstream configFile((path + ".conf"), std::ios::app);
    int numberOfBlocks = fileSize/client.pieceSize;

    if(fileSize%client.pieceSize != 0)
        configFile << numberOfBlocks + 1;
    else
        configFile << numberOfBlocks;
       
    configFile << std::endl;
    configFile.close();
}

void Client::FileManager::removeFragmentedFiles()
{
    DIR *fileDir = opendir(fileDirName);
    
    if(fileDir == NULL)
        throw FileManagerException ("opendir call failed");

    struct dirent *x;
    while( (x=readdir(fileDir)) != NULL )
    {
        const std::string fileName = x->d_name;
        if(fileName != "." && fileName != ".." && fileName.find(".conf") != std::string::npos)
            removeFileIfFragmented(fileName.substr(0, fileName.size()-5));
    }
    closedir(fileDir);    
}

std::vector<char> Client::FileManager::getBlockBytes(Client& client, const std::string& fileName, const int& index)
{
    std::string path = std::string(fileDirName) + "/" + fileName;
    std::fstream file;
    file.open(path);
    
    if(!file.is_open())
        throw FileManagerException("File: " + path + " could not be opened.");

	off_t offset = index * client.pieceSize;
	file.seekp(long(offset), std::ios_base::beg);

    int diff = getFileSize(fileName) - offset;
    int thisBlockSize = (diff < client.pieceSize) ? diff : client.pieceSize;

    std::vector<char> bytes(thisBlockSize);
    file.read(&bytes[0], thisBlockSize);

    file.close();
    return bytes;
}

bool Client::FileManager::doesBlockExist(Client& client, const std::string& fileName, const int& index)
{
    std::fstream file;
    file.open(std::string(fileDirName) + "/" + fileName + ".conf");

    if(file.is_open())
    {
        std::istream_iterator<int> begin(file);
        std::istream_iterator<int> end;
        return std::find(++begin, end, index) != end;         // poszukiwanie od 2 liczby, bo początek to docelowa liczba bloków
    }
    else
    {
        return index * client.pieceSize < getFileSize(fileName);
    }
    
}

std::vector<int> Client::FileManager::getIndexesFromConfig(const std::string& fileName)
{
    std::string path = std::string(fileDirName) + "/" + fileName + ".conf";
    std::fstream file;
    file.open(path);

    if(!file.is_open())
        throw FileManagerException ("File: " + path + " could not be opened.");
    
    std::vector<int> indexes;
    int index;
    file >> index;

    while(file >> index)
        indexes.push_back(index);

    return indexes;
}

void Client::FileManager::copyFile(const std::string& absoluteFilePath, const std::string& newFileName)
{
    std::ifstream oldFile;
    oldFile.open(absoluteFilePath);

    if(!oldFile.is_open())
    {
        std::cerr << absoluteFilePath << " " << " could not be found" << std::endl;
        return;
    }

    std::ofstream newFile((std::string(fileDirName) + "/" + newFileName), std::ios::app);

    newFile << oldFile.rdbuf();
}

bool Client::FileManager::isFileComplete(const std::string& fileName)
{
    std::string path = std::string(fileDirName) + "/" + fileName + ".conf";
    std::fstream file;
    file.open(path);

    if(!file.is_open())
        throw FileManagerException ("File: " + path + " could not be opened.");

    int defaultBlocksNumber;
    file.seekg(0);
    file >> defaultBlocksNumber;
    return std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n') - 1 == defaultBlocksNumber;   
}

void Client::FileManager::removeConfig(const std::string& fileName)
{
    std::string path = std::string(fileDirName) + "/" + fileName + ".conf";
    remove((path).c_str());
}

FileManagerException::FileManagerException(const std::string& msg) : info("FileManager Exception: " + msg) {}

const char* FileManagerException::what() const throw()
{
    return info.c_str();
}
