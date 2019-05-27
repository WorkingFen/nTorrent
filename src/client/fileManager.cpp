#include "headers/fileManager.hpp"
#include <libgen.h>
#include <iostream>

Client::FileManager::FileManager()
{
    char result[4096];
    if(readlink("/proc/self/exe", result, PATH_MAX) == -1)
        throw FileManagerException("Could not find executable's location!");

    strcpy(fileDirName, dirname(result));
    seedsDirName = std::string(fileDirName) + "/seeds";
    outputDirName = std::string(fileDirName) + "/output";
}

Client::FileManager::~FileManager() {}

void Client::FileManager::removeFileIfFragmented(const std::string& fileName)
{
    const std::string configPath = seedsDirName + "/configFiles/" + fileName + ".conf";
    const std::string path = seedsDirName + "/" + fileName;

    if(getNumberOfDownloadedBlocks(fileName) < getDefaultNumberOfBlocks(fileName))
    {
        remove(path.c_str());
    }
    remove((configPath).c_str());
}

int Client::FileManager::getNumberOfDownloadedBlocks(const std::string& fileName)
{
    const std::string configPath = seedsDirName + "/configFiles/" + fileName + ".conf";

    std::ifstream file;
    file.open(configPath);

    if(!file.is_open())
        throw FileManagerException ("File: " + configPath + " could not be opened.");

    return std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n') - 1;
}

int Client::FileManager::getDefaultNumberOfBlocks(const std::string& fileName)
{
    const std::string configPath = seedsDirName + "/configFiles/" + fileName + ".conf";

    std::ifstream file;
    file.open(configPath);

    if(!file.is_open())
        throw FileManagerException ("File: " + configPath + " could not be opened.");
    
    int defaultBlocksNumber;
    file.seekg(0);
    file >> defaultBlocksNumber;

    return defaultBlocksNumber;
}

bool Client::FileManager::doesFileExist(const std::string& fileName, const bool isConfig)
{
    std::string seekedFile = fileName;
    DIR *fileDir;
    if(isConfig)
    {
        fileDir = opendir((seedsDirName + "/configFiles").c_str());
        seekedFile += ".conf";
    }
    else
       fileDir = opendir((seedsDirName).c_str());
    
    if(fileDir == NULL)
        throw FileManagerException ("opendir call failed");

    struct dirent *x;
    while( (x=readdir(fileDir)) != NULL )
    {
        const std::string name = x->d_name;
        if(name == fileName)
        {
            closedir(fileDir);
            return true;
        }
    }
    closedir(fileDir);
    return false;
}

void Client::FileManager::printOutputFolderContent()
{
    DIR *fileDir = opendir(outputDirName.c_str());
    
    if(fileDir == NULL)
        throw FileManagerException ("opendir call failed");

    struct dirent *x;
    int i=1;
    std::cout << std::endl << "ZAWARTOSC KATALOGU OUTPUT:" << std::endl; 
    while( (x=readdir(fileDir)) != NULL )
    {
        const std::string name = x->d_name;
        if(name != "." && name != "..")
        {
            std::cout << i << ". " << name << std::endl;
            i++;
        }
    }
    std::cout << std::endl;
    closedir(fileDir);
}

void Client::FileManager::printSeedsFolderContent()
{
    DIR *fileDir = opendir(seedsDirName.c_str());
    
    if(fileDir == NULL)
        throw FileManagerException ("opendir call failed");

    struct dirent *x;
    int i=1;
    std::cout << std::endl << "ZAWARTOSC KATALOGU:" << std::endl; 
    while( (x=readdir(fileDir)) != NULL )
    {
        const std::string name = x->d_name;
        if(name != "." && name != ".." && name != "configFiles")
        {
            if(doesFileExist(name, true))
                std::cout << i << ". " << name << " (" << getNumberOfDownloadedBlocks(name) << "/" << getDefaultNumberOfBlocks(name) << ")" << std::endl;
            else
                std::cout << i << ". " << name << " (fully downloaded)" << std::endl;
            
            i++;
        }
    }
    std::cout << std::endl;
    closedir(fileDir);
}

std::vector<std::string> Client::FileManager::getDirFiles()
{
    DIR *fileDir = opendir(seedsDirName.c_str());
    if(fileDir == NULL)
        throw FileManagerException ("opendir call failed");  

    struct dirent *x;
    std::vector<std::string> fileNames;
    while( (x=readdir(fileDir)) != NULL )
    {
        const std::string name = x->d_name;
        if(name != "." && name != ".." && name != "configFiles")
        {
            fileNames.push_back(name);
        }
    }
    closedir(fileDir);
    return fileNames;
} 

off_t Client::FileManager::getFileSize(const std::string& filename)
{
    struct stat fileStats;
    const std::string filePath = seedsDirName + "/" + filename;

    if(stat(filePath.c_str(), &fileStats) == -1)
        throw FileManagerException("Error while retrieving info about file!");
    
    return fileStats.st_size;
}

void Client::FileManager::putPiece(Client& client, const std::string& fileName, const int& index, const std::string& pieceData) 
{
    const std::string path = seedsDirName + "/" + fileName;

	std::fstream filePieces;
    filePieces.open(path);

    if(!filePieces.is_open())
        throw FileManagerException ("File: " + path + " could not be opened.");

	const off_t offset = index * client.pieceSize;
	filePieces.seekp(long(offset), std::ios_base::beg);

	filePieces << pieceData;

	filePieces.close();

    const std::string configPath = seedsDirName + "/configFiles/" + fileName + ".conf";

	std::ofstream configFile;
    configFile.open(configPath, std::ios::app);

    if(!configFile.is_open())
        throw FileManagerException ("File: " + configPath + " could not be opened.");

	configFile << index;
	configFile << std::endl;

    configFile.close();
}

void Client::FileManager::createConfig(Client& client, const std::string& fileName, const off_t& fileSize)
{
    std::ofstream newFile(seedsDirName + "/" + fileName, std::ios::ate);             // tworzy nowy plik

    newFile.seekp(fileSize - 1);
    newFile.write("",1);
    newFile.seekp(0);
    newFile.close();

    std::ofstream configFile((seedsDirName + "/configFiles/" + fileName + ".conf"), std::ios::app);
    const int numberOfBlocks = fileSize/client.pieceSize;

    if(fileSize%client.pieceSize != 0)
        configFile << numberOfBlocks + 1;
    else
        configFile << numberOfBlocks;
       
    configFile << std::endl;
    configFile.close();
}

void Client::FileManager::removeFragmentedFiles()
{
    const std::string configDirName = seedsDirName + "/configFiles";

    DIR *fileDir = opendir(configDirName.c_str());
    
    if(fileDir == NULL)
        throw FileManagerException ("opendir call failed");

    struct dirent *x;
    while( (x=readdir(fileDir)) != NULL )
    {
        const std::string fileName = x->d_name;
        if(fileName != "." && fileName != "..")
            removeFileIfFragmented(fileName.substr(0, fileName.size()-5));
    }
    closedir(fileDir);    
}

std::vector<char> Client::FileManager::getBlockBytes(Client& client, const std::string& fileName, const int& index)
{
    const std::string path = seedsDirName + "/" + fileName;
    std::fstream file;
    file.open(path);
    
    if(!file.is_open())
        throw FileManagerException("File: " + path + " could not be opened.");

	const off_t offset = index * client.pieceSize;
	file.seekp(long(offset), std::ios_base::beg);

    const int diff = getFileSize(fileName) - offset;
    const int thisBlockSize = (diff < client.pieceSize) ? diff : client.pieceSize;

    std::vector<char> bytes(thisBlockSize);
    file.read(&bytes[0], thisBlockSize);

    file.close();
    return bytes;
}

bool Client::FileManager::doesBlockExist(Client& client, const std::string& fileName, const int& index)
{
    std::fstream file;
    file.open(seedsDirName + "/configFiles/" + fileName + ".conf");

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
    const std::string path = seedsDirName + "/configFiles/" + fileName + ".conf";
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

void Client::FileManager::copyFile(const std::string& filePath, const std::string& newFileName)
{
    if(doesFileExist(newFileName, false))
    {
        std::cerr << newFileName << " already exists in this location!" << std::endl;
        return;
    }

    std::ifstream oldFile;

    if(filePath.size() >= 2 && filePath.substr(0,2) == "./")
    {
        oldFile.open(outputDirName + filePath.substr(1, filePath.size()-1));
    }
    else
    {
        oldFile.open(filePath);
    }

    if(!oldFile.is_open())
    {
        std::cerr << filePath << " could not be found" << std::endl;
        return;
    }

    std::ofstream newFile((seedsDirName + "/" + newFileName), std::ios::app);

    newFile << oldFile.rdbuf();
}

bool Client::FileManager::isFileComplete(const std::string& fileName)
{
    return getDefaultNumberOfBlocks(fileName) == getDefaultNumberOfBlocks(fileName);   
}

void Client::FileManager::removeConfig(const std::string& fileName)
{
    const std::string path = seedsDirName + "/configFiles/" + fileName + ".conf";
    remove((path).c_str());
}

FileManagerException::FileManagerException(const std::string& msg) : info("FileManager Exception: " + msg) {}

const char* FileManagerException::what() const throw()
{
    return info.c_str();
}
