#include "headers/client.hpp"
#include "../hashing/headers/hash.h"
#include "../hashing/headers/sha256.h"
#include <stdexcept>
#include <string.h>
#include <algorithm>
#include <string>
#include <ctime>
#include <fstream>

#include "headers/fileManager.hpp"
#include "headers/consoleInterface.hpp"

using namespace msg;

Client::Client(const char ipAddr[15], const int& port, const char serverIpAddr[15], const int& serverPort) : maxFd(0), console(std::unique_ptr<ConsoleInterface>(new ConsoleInterface)), fileManager(std::unique_ptr<FileManager>(new FileManager))
{
    prepareSockaddrStruct(self, ipAddr, port);
    prepareSockaddrStruct(server, serverIpAddr, serverPort);

    if ((sockFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        throw ClientException("socket call failed");

    maxFd = sockFd + 1;

    int enable = 1;
    if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        std::cerr << "setting SO_REUSEADDR option on socket " << sockFd << " failed" << std::endl;

    if (bind(sockFd, (struct sockaddr *)&self, sizeof self) == -1)
        throw ClientException("bind call failed");

    if (listen(sockFd, 20) == -1)
        throw ClientException("listen call failed");

    char fileDir[1000];
    getcwd(fileDir, 1000);
    strcat(fileDir, "/clientFiles");

    if (port == 0)
    {
        struct sockaddr_in x;
        socklen_t y = sizeof(self);
        getsockname(sockFd, (struct sockaddr *)&x, &y);
        this->port = (int)ntohs(x.sin_port);
        self.sin_port = x.sin_port;
    }
    else
    {
        this->port = port;
    }
    std::cout << "Successfully connected and listening at: " << ipAddr << ":" << this->port << std::endl;
    fileManager->removeFragmentedFiles();
}

Client::~Client()
{
    std::cout << "Disconnected" << std::endl;
}

void Client::connectTo(const struct sockaddr_in &address)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(sock, (struct sockaddr *)&address, sizeof address) == -1) // TODO obsługa błędów - dodać poprawne zamknięcie
        throw ClientException("connect call failed");

    std::cout << "Connected to sbd" << std::endl;

    if (address.sin_addr.s_addr == server.sin_addr.s_addr)
        mainServerSocket = sock;
    else
    {
        seederSockets.push_back(FileSocket(sock));              // TODO obsługa błędów
    }

    maxFd = std::max(maxFd, sock + 1);

    sendListeningAddress();
}

const struct sockaddr_in &Client::getServer() const
{
    return server;
}

void Client::signal_waiter()
{
    int sig_number;

    sigwait(&signal_set, &sig_number);
    if (sig_number == SIGINT)
    {
        std::cout << "\rReceived SIGINT. Exiting..." << std::endl; // tylko do debugowania
        interrupted_flag = true;
    }
}

void Client::setSigmask()
{
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGINT);
    int status = pthread_sigmask(SIG_BLOCK, &signal_set, NULL);
    if (status != 0)
        std::cerr << "Setting signal mask failed" << std::endl;
}

void Client::prepareSockaddrStruct(struct sockaddr_in &x, const char ipAddr[15], const int &port)
{
    x.sin_family = AF_INET;
    if ((inet_pton(AF_INET, ipAddr, &x.sin_addr)) <= 0)
        throw std::invalid_argument("Improper IPv4 address: " + std::string(ipAddr));
    x.sin_port = htons(port);
}

void Client::prepareSockaddrStruct(struct sockaddr_in &x, const int ipAddr, const int &port)
{
    x.sin_family = AF_INET;
    x.sin_addr.s_addr = ipAddr;
    x.sin_port = htons(port);
}

void Client::getUserCommands()
{
    if (FD_ISSET(0, &ready))
    {
        char buffer[4096];
        fgets(buffer, 4096, stdin);
        console->processCommands(buffer); // exception?????
    }
}

void Client::handleCommands()
{
    try
    {
        console->handleInput(*this);
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        run_stop_flag = 1;
    }
}

void Client::handleServerFileInfo(msg::Message msg)
{
    if (console->getMessageState() == MessageState::wait_for_file_info) // jeśli czekaliśmy na to info
    {
        int fileNameLength = msg.readInt(); // długość nazwy

        std::string fileName = msg.readString(fileNameLength); // nazwa pliku

        if (fileName != console->getChosenFile()) // TODO: logi
        {
            return;
        }
        int fileSize = msg.readInt(); // rozmiar pliku

        fileManager->createConfig(*this, fileName, fileSize);

        std::vector<int> indexes;//to się wydaje niepotrzebne, skoro config jest pusty
        try
        {
            indexes = fileManager->getIndexesFromConfig(fileName);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            return;
        }//do tego momentu wydaje się niepotrzebne
        
        sendAskForBlock(mainServerSocket, fileName, indexes); // wysyła zapytanie o blok
        //std::cout << 123 << std::endl;

        console->setMessageState(MessageState::wait_for_block_info); // ustaw stan na oczekiwanie na informację skąd pobrać blok
    }
    else
    {
        /* serwer wysłał wiadomość nieodpowiednią dla stanu - albo chciał listować */
        int fileNameLength = msg.readInt(); // długość nazwy

        std::string fileName = msg.readString(fileNameLength); // nazwa pliku
        int fileSize = msg.readInt(); // rozmiar pliku

        std::cout << "File: " << fileName << "\t\t" << "Size: " << fileSize << "b" << std::endl;
    }
}

void Client::handleServerBlockInfo(msg::Message msg)
{
    //if (console->getMessageState() == MessageState::wait_for_block_info) // jeśli czekaliśmy na to info
    //{
        int fileNameLength = msg.readInt();                    // długość nazwy
        std::string fileName = msg.readString(fileNameLength); // nazwa pliku
        int blockIndex = msg.readInt();                        // numer bloku
        std::string hash = msg.readString(64);                 // nazwa pliku
        int address = msg.readInt();                           // adres
        int port = msg.readInt();                              // port
/*
        std::vector<int> indexes = fileManager->getIndexesFromConfig(fileName);

        sendAskForBlock(mainServerSocket, fileName, indexes); */
        console->setMessageState(MessageState::none);

        (void)port;
        (void)address;

        leechFile(134744191,4400,fileName,blockIndex,hash);
    //}
    //else
    //{
        /* serwer wysłał wiadomość nieodpowiednią dla stanu */
    //}
}

void Client::handleSeederFile(FileSocket &s, msg::Message &msg)
{
    std::string hash = hashOnePiece(msg.buffer);
    if(hash != s.hash)
    {
        std::cout << "bad hash:\n" << hash << std::endl << s.hash << std::endl;
        //sendBadBlockHash(mainServerSocket, s.filename, s.);
    }
    else
    {
        fileManager->putPiece(*this, s.filename, s.blockIndex, std::string(msg.buffer.begin(), msg.buffer.end()));
        sendHaveBlock(mainServerSocket, s.filename, s.blockIndex, hash);
    }
    
    msg::Message(111).sendMessage(s.sockFd);
    close(s.sockFd);
    std::string fileName = s.filename;
    //it = seederSockets.erase(it);

    if(fileManager->isFileComplete(fileName))
    {
        std::cout << "Download completed!" << std::endl;
        fileManager->removeConfig(fileName);
        fileManager->moveSeedToOutput(fileName);
    }
    else
        sendAskForBlock(mainServerSocket, fileName, fileManager->getIndexesFromConfig(fileName)); // wysyła zapytanie o kolejny blok
}

void Client::handleMessagesfromServer()
{
    if (mainServerSocket == -1 || !FD_ISSET(mainServerSocket, &ready))
        return;

    if (msg_manager.assembleMsg(mainServerSocket))
    {
        msg::Message msg = msg_manager.readMsg(mainServerSocket);
        if(msg.type != 209) std::cout << "Received from server: " << msg.type << std::endl << std::endl;

        if (msg.type == 211)
        {
            std::cout << "Server disconnected!" << std::endl;
            disconnect();
            console->setState(State::up);
        }
        else if (msg.type == 210)
        {
            pieceSize = msg.readInt();
            std::cout << "Piece size set to " << pieceSize << std::endl;

            shareFiles();
        }
        else if (msg.type == 201) // serwer wysłał info o pliku do pobrania
        {
            handleServerFileInfo(msg);
        }
        else if (msg.type == 202) // serwer wysłał info o bloku do pobrania
        {
            handleServerBlockInfo(msg);
        }
    }
    else if (msg_manager.lastReadResult() == 0 || msg_manager.lastReadResult() == -1) //server left
    {
        std::cout << "Lost connection with server!" << std::endl;
        run_stop_flag = true;
    }
}

void Client::handleMessagesfromSeeders()
{
    for(auto it=seederSockets.begin(); it!=seederSockets.end();)                // pętla dla leecherSockets -> TODO pętla dla cilentSockets
    {
        if(std::time(0) - it->last_activity > timeout)
        {
            msg::Message(111).sendMessage(it->sockFd);

            close(it->sockFd);
            it = seederSockets.erase(it);
        }
        if(FD_ISSET(it->sockFd, &ready))
        {
            it->last_activity = std::time(0);

            if(msg_manager.assembleMsg(it->sockFd))
            {
                msg::Message msg = msg_manager.readMsg(it->sockFd);
                std::cout << "Received from seeder: " << msg.type << std::endl << std::endl;

                if(msg.type == 111)                                // tu msg
                {
                    close(it->sockFd);
                    it = seederSockets.erase(it);

                    std::cout << "Connection severed" << std::endl;
                }
                else if(msg.type == 401)
                {
                    handleSeederFile(*it, msg);
                    it = seederSockets.erase(it);   //blargh
                }
                else if(msg.type == 402)
                {
                    //ej serwer dałeś złego klienta

                    msg::Message(111).sendMessage(it->sockFd);

                    close(it->sockFd);
                    it = seederSockets.erase(it);
                }
                else ++it;
            }
        }

        ++it;
    }
}

void Client::handleMessagesfromLeechers()
{
    for(auto it=leecherSockets.begin(); it!=leecherSockets.end();)                // pętla dla leecherSockets -> TODO pętla dla cilentSockets
    {
        if(std::time(0) - it->last_activity > timeout)
        {
            msg::Message(111).sendMessage(it->sockFd);

            close(it->sockFd);
            it = leecherSockets.erase(it);
        }

        if(FD_ISSET(it->sockFd, &ready))
        {
            it->last_activity = std::time(0);

            if(msg_manager.assembleMsg(it->sockFd))
            {
                msg::Message msg = msg_manager.readMsg(it->sockFd);

                std::cout << "Received from leecher: " << msg.type << std::endl << std::endl;

                if(msg.type == 111)                                // tu msg
                {
                    close(it->sockFd);
                    it = leecherSockets.erase(it);

                    std::cout << "Connection severed" << std::endl;
                }
                else if(msg.type == 301)
                {
                    std::string filename = msg.readString(msg.readInt());
                    int block_index = msg.readInt();

                    it->filename = filename;
                    it->blockIndex = block_index;

                    seedFile(it->sockFd, filename, block_index);
                }
                else ++it;
            }
        }

        ++it;
    }
}

void Client::sendListeningAddress()
{
    msg::Message msg(112);

    msg.writeInt(self.sin_addr.s_addr);
    msg.writeInt(self.sin_port);

    msg.sendMessage(mainServerSocket);
}


void Client::shareFile(std::string directory, std::string fname)
{
    std::vector<std::string> hashes = hashPieces(directory + "/" + fname, pieceSize); //fileDIRname instead of clientFiles

    msg::Message share_msg(101);

    share_msg.writeInt(fname.size()); //name size
    share_msg.writeString(fname);     //file name

    share_msg.writeInt((int)fileManager->getFileSize(fname));

    for (std::string hash : hashes)
        share_msg.writeString(hash); //hash for that piece

    share_msg.sendMessage(mainServerSocket);
}

void Client::shareFiles()
{
    std::vector<std::string> file_names = std::move(fileManager->getDirFiles());

    for (std::string fname : file_names)
        shareFile("seeds", fname);

    if (file_names.size() > 0)
        console->setState(State::seeding);
}

void Client::sendFileInfo(int socket, std::string directory, std::string fname)
{
    std::vector<std::string> hashes = hashPieces(directory + "/" + fname, pieceSize); //fileDIRname instead of clientFiles
    int i = 0;

    for (std::string hash : hashes)
    {
        msg::Message fileinfo(105);

        fileinfo.writeInt(fname.size()); //name size
        fileinfo.writeString(fname);     //file name

        fileinfo.writeInt(i);       //piece number
        fileinfo.writeString(hash); //hash for that piece

        fileinfo.sendMessage(socket); //nalezy wyroznic jakos socket przez ktory komunikujemy sie z serwerem

        ++i;
    }
}

void Client::sendFilesInfo()
{
    std::vector<std::string> file_names = std::move(fileManager->getDirFiles());

    for (std::string fname : file_names)
        sendFileInfo(mainServerSocket, "clientFiles", fname);
}

void Client::sendDeleteBlock(int socket, std::string fileName, int blockIndex)
{
    msg::Message deleteBlock(103);

    deleteBlock.writeInt(fileName.size());
    deleteBlock.writeString(fileName);
    deleteBlock.writeInt(blockIndex);

    deleteBlock.sendMessage(socket);
}

void Client::sendDeleteBlocks(std::string fileName)
{
    std::vector<int> indexes = fileManager->getIndexesFromConfig(fileName);

    for (auto index = indexes.begin(); index != indexes.end(); ++index)
    {
        sendDeleteBlock(mainServerSocket, fileName, *index);
    }

    // tutaj usuwanie pliku?
}

void Client::sendDeleteFile(std::string fileName)
{
    std::cout << "fileDelete" << std::endl;
    msg::Message deleteBlock(103);

    deleteBlock.writeInt(fileName.size());
    deleteBlock.writeString(fileName);

    deleteBlock.sendMessage(mainServerSocket);
}

void Client::sendAskForFile(std::string fileName)
{

    msg::Message askForFile(104);

    askForFile.writeInt(fileName.size());
    askForFile.writeString(fileName);

    askForFile.sendMessage(mainServerSocket);
}

void Client::sendHaveBlock(int socket, std::string fileName, int blockIndex, std::string hash)
{
    msg::Message haveBlock(105);

    haveBlock.writeInt(fileName.size()); // file
    haveBlock.writeString(fileName);

    haveBlock.writeInt(blockIndex); // block index

    haveBlock.writeInt(hash.size()); // hash
    haveBlock.writeString(hash);

    haveBlock.sendMessage(socket);
}

void Client::sendAskForBlock(int socket, std::string fileName, vector<int> blockList)
{
    msg::Message askForBlock(106);

    askForBlock.writeInt(fileName.size()); // file
    askForBlock.writeString(fileName);

    for (auto it = blockList.begin(); it != blockList.end(); ++it) // owned block indexes
        askForBlock.writeInt(*it);

    askForBlock.sendMessage(socket);
}

void Client::sendBadBlockHash(int socket, std::string fileName, int blockIndex, int seederAddress, int seederPort)
{
    msg::Message badBlockHash(107);

    badBlockHash.writeInt(fileName.size()); // file
    badBlockHash.writeString(fileName);

    badBlockHash.writeInt(blockIndex); // block

    badBlockHash.writeInt(seederAddress); // ip
    badBlockHash.writeInt(seederPort);    // port

    badBlockHash.sendMessage(socket);
}

void Client::listServerFiles()
{
    msg::Message(102).sendMessage(mainServerSocket);
}

void Client::leechFile(const int ipAddr, int port, std::string filename, int blockIndex, std::string hash)
{
    struct sockaddr_in x;
    prepareSockaddrStruct(x, ipAddr, port);   

    connectTo(x);

    seederSockets.back().filename = filename;
    seederSockets.back().blockIndex = blockIndex;
    seederSockets.back().hash = hash;

    msg::Message leechMsg(301);

    leechMsg.writeInt(filename.size());         // file
    leechMsg.writeString(filename);

    leechMsg.writeInt(blockIndex);              // block    

    leechMsg.sendMessage(seederSockets.back().sockFd);    
}

void Client::seedFile(int socket, std::string filename, int blockIndex)
{
    if(fileManager->doesBlockExist(*this, filename, blockIndex))
    {
        msg::Message seedMsg(401);

        seedMsg.buffer = fileManager->getBlockBytes(*this, filename, blockIndex);
        seedMsg.buf_length = seedMsg.buffer.size();

        seedMsg.sendMessage(socket);
    }
    else
    {
        msg::Message seedMsg(402);

        seedMsg.writeInt(filename.size());
        seedMsg.writeString(filename);
        seedMsg.writeInt(blockIndex);

        seedMsg.sendMessage(socket);
    }
}

void Client::disconnect()
{
    int result;

    if (mainServerSocket != -1) // sprawdzenie czy socket jest używany
    {
        msg::Message(111).sendMessage(mainServerSocket);
        result = close(mainServerSocket);
        if (result == -1)
            std::cerr << "Closing " << mainServerSocket << " socket failed" << std::endl;
        mainServerSocket = -1;
    }

    for(auto it = seederSockets.begin(); it != seederSockets.end(); ++it)
    {  // zamknięcie socketów
		std::cout<<"Closing client socket ("<<it->sockFd<<")"<<std::endl;

        msg::Message(111).sendMessage(it->sockFd);
        result = close(it->sockFd);
        it = seederSockets.erase(it);

        if(result == -1)
        {
            std::cerr<<"Closing "<<it->sockFd<<" socket failed"<<std::endl;
            continue;
        }
    }

    for(auto it = leecherSockets.begin(); it != leecherSockets.end(); ++it)
    {
		std::cout<<"Closing server socket ("<<it->sockFd<<")"<<std::endl;

        msg::Message(111).sendMessage(it->sockFd);
        result = close(it->sockFd);
        it = leecherSockets.erase(it);

        if(result == -1){
            std::cerr<<"Closing "<<it->sockFd<<" socket failed"<<std::endl;
            continue;
        }
        std::cout << "losed " << it->sockFd << " server socket" << std::endl;
    }
    maxFd = sockFd + 1;
    fileManager->removeFragmentedFiles();
}

void Client::turnOff()
{
    pthread_kill(signal_thread.native_handle(), SIGINT); // ubicie ewentualnego sigwaita
    signal_thread.join();
    disconnect();
}

void Client::setFileDescrMask()
{
    FD_ZERO(&ready);

    FD_SET(0, &ready);
    FD_SET(sockFd, &ready);

    if (mainServerSocket != -1)
        FD_SET(mainServerSocket, &ready);

    for(const auto &i : seederSockets)
        FD_SET(i.sockFd, &ready);

    for(const auto &i : leecherSockets)
        FD_SET(i.sockFd, &ready);
}

void Client::run()
{
    setSigmask();                                              // Set sigmask for all threads
    signal_thread = std::thread(&Client::signal_waiter, this); // Create the sigwait thread

    std::time_t keep_alive_timer = std::time(0);

    while (!interrupted_flag && !run_stop_flag && console->getState() != State::down)
    {
        setFileDescrMask();

        to.tv_sec = 1;
        to.tv_usec = 0;

        if ((select(maxFd, &ready, (fd_set *)0, (fd_set *)0, &to)) == -1)
            throw ClientException("select call failed");

        if (FD_ISSET(sockFd, &ready) && leecherSockets.size()<5)
        {
            int newCommSock = accept(sockFd, (struct sockaddr *)0, (socklen_t *)0);
            leecherSockets.push_back(newCommSock);
            maxFd = std::max(maxFd,newCommSock+1);
            std::cout << "leecherSocketsNum = " << leecherSockets.size() << std::endl;
        }

        handleMessagesfromServer();
        handleMessagesfromSeeders();
        handleMessagesfromLeechers();
        getUserCommands();
        handleCommands();

        if (std::time(0) - keep_alive_timer > 5) //keep_alive co 5s
        {
            if (mainServerSocket != -1)
                msg::Message(110).sendMessage(mainServerSocket);
            keep_alive_timer = std::time(0);
        }
    }
    turnOff();
}

ClientException::ClientException(const std::string &msg) : info(msg) {}

const char *ClientException::what() const throw()
{
    return info.c_str();
}
