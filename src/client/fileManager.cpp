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
    const std::string configPath = std::string(fileDirName) + "/configFiles/" + fileName + ".conf";
    const std::string path = std::string(fileDirName) + "/" + fileName;

    std::ifstream inFile;
    inFile.open(configPath);

    if(!inFile.is_open())
        throw FileManagerException ("File: " + configPath + " could not be opened.");

    const int downloadedBlocksNumber = std::count(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>(), '\n') - 1;
    int defaultBlocksNumber;
    inFile.seekg(0);
    inFile >> defaultBlocksNumber;
    
    remove((configPath).c_str());
    if(downloadedBlocksNumber < defaultBlocksNumber)
    {
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
        const std::string name = x->d_name;
        if(name != "." && name != ".." && name != "configFiles")
        {
            std::cout << i << ". " << name << std::endl;
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
    const std::string filePath = std::string(fileDirName) + "/" + filename;

    if(stat(filePath.c_str(), &fileStats) == -1)
        throw FileManagerException("Error while retrieving info about file!");
    
    return fileStats.st_size;
}

void Client::FileManager::putPiece(Client& client, const std::string& fileName, const int& index, const std::string& pieceData) 
{
    const std::string path = std::string(fileDirName) + "/" + fileName;

	std::fstream filePieces;
    filePieces.open(path);

    if(!filePieces.is_open())
        throw FileManagerException ("File: " + path + " could not be opened.");

	const off_t offset = index * client.pieceSize;
	filePieces.seekp(long(offset), std::ios_base::beg);

	filePieces << pieceData;

	filePieces.close();

    const std::string configPath = std::string(fileDirName) + "/configFiles/" + fileName + ".conf";

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
    const std::string path(fileDirName);

    std::ofstream newFile(path + "/" + fileName, std::ios::ate);             // tworzy nowy plik

    newFile.seekp(fileSize - 1);
    newFile.write("",1);
    newFile.seekp(0);
    newFile.close();

    std::ofstream configFile((path + "/configFiles/" + fileName + ".conf"), std::ios::app);
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
    const std::string configDirName = std::string(fileDirName) + "/configFiles";

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
    const std::string path = std::string(fileDirName) + "/" + fileName;
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
    file.open(std::string(fileDirName) + "/configFiles/" + fileName + ".conf");

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
    const std::string path = std::string(fileDirName) + "/configFiles/" + fileName + ".conf";
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
    const std::string path = std::string(fileDirName) + "/configFiles/" + fileName + ".conf";
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
    const std::string path = std::string(fileDirName) + "/configFiles/" + fileName + ".conf";
    remove((path).c_str());
}

FileManagerException::FileManagerException(const std::string& msg) : info("FileManager Exception: " + msg) {}

const char* FileManagerException::what() const throw()
{
    return info.c_str();
}
