#include "headers/client.hpp"
#include "../hashing/headers/hash.h"
#include "../hashing/headers/sha256.h"
#include <stdexcept>
#include <string.h>
#include <algorithm>
#include <string>
#include <ctime>
#include <fstream>

#include "headers/consoleInterface.hpp"

using namespace msg;

Client::Client(const char ipAddr[15], const int& port, const char serverIpAddr[15], const int& serverPort) : clientSocketsNum(0), serverSocketsNum(0), maxFd(0), console(std::unique_ptr<ConsoleInterface>(new ConsoleInterface)), fileManager(FileManagerPtr(new FileManager))
{
    prepareSockaddrStruct(self, ipAddr, port);
    prepareSockaddrStruct(server, serverIpAddr, serverPort);

    if( (sockFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        throw std::runtime_error ("socket call failed");

    maxFd = sockFd+1;

    int enable = 1;
    if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        std::cerr<<"setting SO_REUSEADDR option on socket "<<sockFd<<" failed"<<std::endl;   

    if(bind(sockFd, (struct sockaddr *) &self, sizeof self) == -1)
        throw std::runtime_error ("bind call failed");

    if(listen(sockFd, 20) == -1)
        throw std::runtime_error ("listen call failed");

    char fileDir[1000];
    getcwd(fileDir, 1000);
    strcat(fileDir, "/clientFiles");

    if(port == 0)
    {
        struct sockaddr_in x;
        socklen_t y = sizeof(self);
        getsockname(sockFd, (struct sockaddr *) &x, &y);
        this->port = (int) ntohs(x.sin_port);
        self.sin_port = x.sin_port;
    }
    else
    {
        this->port=port;
    }
    std::cout << "Successfully connected and listening at: " << ipAddr << ":" << this->port << std::endl;
}


Client::~Client()
{
    std::cout << "Disconnected" << std::endl;  
}

void Client::connectTo(const struct sockaddr_in &address)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if(connect(sock, (struct sockaddr *) &address, sizeof address) == -1)         // TODO obsługa błędów - dodać poprawne zamknięcie
        throw std::runtime_error ("connect call failed");

    std::cout << "Connected to sbd" << std::endl;

    if(address.sin_addr.s_addr == server.sin_addr.s_addr)
        mainServerSocket = sock;
    else 
    {
        clientSocketsNum++;
        clientSockets.push_back(sock);              // TODO obsługa błędów
    }

    maxFd = std::max(maxFd,sock+1);    
}

const struct sockaddr_in& Client::getServer() const
{
    return server;
}

void Client::signal_waiter()
{
	int sig_number;

    sigwait (&signal_set, &sig_number);
    if (sig_number == SIGINT) 
	{
        std::cout<<"\rReceived SIGINT. Exiting..."<<std::endl;      // tylko do debugowania
        interrupted_flag = true;
	}
}

void Client::setSigmask()
{

    sigemptyset (&signal_set);
    sigaddset (&signal_set, SIGINT);
    int status = pthread_sigmask (SIG_BLOCK, &signal_set, NULL);
    if (status != 0)
        std::cerr<<"Setting signal mask failed"<<std::endl;
}

void Client::prepareSockaddrStruct(struct sockaddr_in& x, const char ipAddr[15], const int& port)
{
    x.sin_family = AF_INET;
    if( (inet_pton(AF_INET, ipAddr, &x.sin_addr)) <= 0)
        throw std::invalid_argument("Improper IPv4 address: " + std::string(ipAddr));
    x.sin_port = htons(port);
}

void Client::getUserCommands()
 {
    if(FD_ISSET(0, &ready))
    {
        char buffer[4096];
        fgets(buffer, 4096, stdin);
        console->processCommands(buffer);        // exception?????
    }
 }

void Client::handleCommands()
{
    try
    {
        console->handleInput(*this);          // aby nie zezwolić wątkowi na próbę wypisania menu
    }
    catch(std::exception& e) 
    { 
        std::cerr << e.what() << std::endl;   
        run_stop_flag=1;
    }
}

void Client::handleMessagesfromServer()
{
    if(mainServerSocket == -1 || !FD_ISSET(mainServerSocket, &ready)) return;

    if(msg_manager.assembleMsg(mainServerSocket))
    {
        msg::Message msg = msg_manager.readMsg(mainServerSocket);
        std::cout << "Received from server: " << msg.type << std::endl;

        if(msg.type == 211)                                // tu msg
        {
            std::cout << "Server disconnected!" << std::endl;
            run_stop_flag = true;
        }
        else if(msg.type == 210)
        {
            pieceSize = msg.readInt();
            std::cout << "Piece size set to " << pieceSize << std::endl;

            shareFiles();
        }
    }
    else if(msg_manager.lastReadResult() == 0 || msg_manager.lastReadResult() == -1) //server left
    {
        std::cout << "Lost connection with server!" << std::endl;
        run_stop_flag = true;
    }

}

void Client::handleMessages()
{
    for(auto it=serverSockets.begin(); it!=serverSockets.end();)                // pętla dla serverSockets -> TODO pętla dla cilentSockets
    {
        if(FD_ISSET(*it, &ready) && msg_manager.assembleMsg(*it))
        {
            msg::Message msg = msg_manager.readMsg(*it);

            std::cout << msg.type << std::endl;
            if(msg.type == 111)                                // tu msg
            {
                close(*it);
                it = serverSockets.erase(it);
                serverSocketsNum--;

                std::cout << "Connection severed" << std::endl;
            }
            else if(msg.type == 301)
            {
                std::string filename = msg.readString(msg.readInt());
                int block_index = msg.readInt();

                seedFile(*it, filename, block_index);
            }
            else
            {
                ++it;
            }
        }
        else
        {
            ++it;
        }
    }
}

void Client::shareFile(int socket, std::string directory, std::string fname)
{
    std::vector<std::string> hashes = hashPieces(directory + "/" + fname, pieceSize);    //fileDIRname instead of clientFiles
    
    msg::Message share_msg(101);

    share_msg.writeInt(fname.size());    //name size
    share_msg.writeString(fname);        //file name

    share_msg.writeInt((int)fileManager->getFileSize(fname));

    for(std::string hash : hashes)
        share_msg.writeString(hash);         //hash for that piece

    share_msg.sendMessage(socket);
}

void Client::shareFiles()
{
    std::vector<std::string> file_names = std::move(fileManager->getDirFiles());

    for(std::string fname : file_names) shareFile(mainServerSocket, "clientFiles", fname);
}

void Client::sendFileInfo(int socket, std::string directory, std::string fname)
{
    std::vector<std::string> hashes = hashPieces(directory + "/" + fname, pieceSize);    //fileDIRname instead of clientFiles
    int i=0;
    
    for(std::string hash : hashes)
    {
        msg::Message fileinfo(105);

        fileinfo.writeInt(fname.size());    //name size
        fileinfo.writeString(fname);        //file name

        fileinfo.writeInt(i);               //piece number
        fileinfo.writeString(hash);         //hash for that piece

        fileinfo.sendMessage(socket);   //nalezy wyroznic jakos socket przez ktory komunikujemy sie z serwerem

        ++i;
    }    
}

void Client::sendFilesInfo()
{
    std::vector<std::string> file_names = std::move(fileManager->getDirFiles());

    for(std::string fname : file_names) sendFileInfo(mainServerSocket, "clientFiles", fname);
}

void Client::sendDeleteBlock(int socket, std::string fileName, int blockIndex)
{
    msg::Message deleteBlock(103);

    deleteBlock.writeInt(fileName.size());
    deleteBlock.writeString(fileName);
    deleteBlock.writeInt(blockIndex);  

    deleteBlock.sendMessage(socket);        
}

void Client::sendAskForFile(int socket, std::string fileName)
{
    msg::Message askForFile(104);

    askForFile.writeInt(fileName.size());
    askForFile.writeString(fileName);

    askForFile.sendMessage(socket);
}

void Client::sendHaveBlock(int socket, std::string fileName, int blockIndex, std::string hash)
{
    msg::Message haveBlock(105);

    haveBlock.writeInt(fileName.size());    // file
    haveBlock.writeString(fileName);

    haveBlock.writeInt(blockIndex);         // block index

    haveBlock.writeInt(hash.size());        // hash
    haveBlock.writeString(hash);

    haveBlock.sendMessage(socket);
}

void Client::sendAskForBlock(int socket, std::string fileName, vector<int> blockList)
{
    msg::Message askForBlock(106);

    askForBlock.writeInt(fileName.size());    // file
    askForBlock.writeString(fileName);

    for(auto it = blockList.begin(); it != blockList.end(); ++it) // owned block indexes
        askForBlock.writeInt(*it);            

    askForBlock.sendMessage(socket);
}

void Client::sendBadBlockHash(int socket, std::string fileName, int blockIndex, std::string seederAddress)
{
    msg::Message badBlockHash(107);

    badBlockHash.writeInt(fileName.size());         // file
    badBlockHash.writeString(fileName);

    badBlockHash.writeInt(blockIndex);              // block

    badBlockHash.writeInt(seederAddress.size());    // address
    badBlockHash.writeString(seederAddress);        

    badBlockHash.sendMessage(socket);
}

void Client::leechFile(int socket, std::string filename, int blockIndex)
{
    msg::Message leechMsg(301);

    leechMsg.writeInt(filename.size());         // file
    leechMsg.writeString(filename);

    leechMsg.writeInt(blockIndex);              // block    

    leechMsg.sendMessage(socket);    
}

void Client::seedFile(int socket, std::string filename, int blockIndex)
{
    msg::Message seedMsg(401);

    seedMsg.buffer = {'e','x','a','m','p','l','e'};
    seedMsg.buf_length = seedMsg.buffer.size();

    seedMsg.sendMessage(socket);
}

void Client::turnOff()
{
	pthread_kill(signal_thread.native_handle(), SIGINT);    // ubicie ewentualnego sigwaita
    signal_thread.join();

    msg::Message(111).sendMessage(mainServerSocket);
    int result = close(mainServerSocket);
    if(result == -1)
        std::cerr<<"Closing "<< mainServerSocket <<" socket failed"<<std::endl;

    for(auto it = clientSockets.begin(); it != clientSockets.end(); ++it){  // zamknięcie socketów
		std::cout<<"Closing client socket ("<<*it<<")"<<std::endl;

        msg::Message(111).sendMessage(*it);
        result = close(*it);
        if(result == -1){
            std::cerr<<"Closing "<<*it<<" socket failed"<<std::endl;
            continue;
        }
    }

    for(auto it = serverSockets.begin(); it != serverSockets.end(); ++it){
		std::cout<<"Closing server socket ("<<*it<<")"<<std::endl;

        msg::Message(111).sendMessage(*it);
        result = close(*it);

        if(result == -1){
            std::cerr<<"Closing "<<*it<<" socket failed"<<std::endl;
            continue;
        }
        std::cout<<"losed "<<*it<<" server socket"<<std::endl;
    }
}

void Client::setFileDescrMask()
{
    FD_ZERO(&ready);

    FD_SET(0, &ready);
    FD_SET(sockFd, &ready);

    if(mainServerSocket != -1) FD_SET(mainServerSocket, &ready);

    for(const int &i : clientSockets)
        FD_SET(i, &ready);

    for(const int &i : serverSockets)
        FD_SET(i, &ready);
}

void Client::run()
{
    setSigmask();       // Set sigmask for all threads
    signal_thread = std::thread(&Client::signal_waiter, this);        // Create the sigwait thread

    std::time_t keep_alive_timer = std::time(0);

    while(!interrupted_flag && !run_stop_flag && console->getState() != State::down)
    {
        setFileDescrMask();
        
        to.tv_sec = 1;
        to.tv_usec = 0;

        if ( (select(maxFd, &ready, (fd_set *)0, (fd_set *)0, &to)) == -1)
             throw std::runtime_error ("select call failed");

        if (FD_ISSET(sockFd, &ready) && serverSocketsNum<5)
        {
            int newCommSock = accept(sockFd, (struct sockaddr *)0, (socklen_t *)0);
            serverSockets.push_back(newCommSock);
            serverSocketsNum++;
            maxFd = std::max(maxFd,newCommSock+1);
            std::cout << "serverSocketsNum = " << serverSocketsNum << std::endl;
        }

        handleMessagesfromServer();
        handleMessages();
        getUserCommands();
        handleCommands();

        if(std::time(0) - keep_alive_timer > 5)    //keep_alive co 5s
        {
            if(mainServerSocket != -1) msg::Message(110).sendMessage(mainServerSocket);
            keep_alive_timer = std::time(0);
        }

    }
        turnOff();
}



