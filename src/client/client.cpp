#include "headers/client.hpp"
#include "../hashing/headers/hash.h"
#include "../hashing/headers/sha256.h"
#include <stdexcept>
#include <string.h>
#include <algorithm>
#include <string>

using namespace msg;

Client::Client(const char ipAddr[15], const int& port, const char serverIpAddr[15], const int& serverPort) : clientSocketsNum(0), serverSocketsNum(0), maxFd(0), endFlag(0)
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

void Client::input_thread()
{
    std::string input;
    std::unique_lock<std::mutex> lock(inputLock);
    while(!interrupted_flag && console->getState() != State::down )
    {
        condition.wait(lock);

        if(endFlag==0)
        {
            console->printMenu();
        }
        else
        {
            break;
        }
        
        std::cin>>input;
        //pthread_mutex_lock(&input_lock);
        commandLock.lock();

        try{
            command = std::stoi(input);
        } catch(std::exception& e) { command=-1;}       // -1 dla inputHandler
        commandLock.unlock();
        //pthread_mutex_unlock(&input_lock);
    }
    std::cout << "input end" << std::endl;
}

void Client::prepareSockaddrStruct(struct sockaddr_in& x, const char ipAddr[15], const int& port)
{
    x.sin_family = AF_INET;
    if( (inet_pton(AF_INET, ipAddr, &x.sin_addr)) <= 0)
        throw std::invalid_argument("Improper IPv4 address: " + std::string(ipAddr));
    x.sin_port = htons(port);
}

void Client::turnOff()
{
    int result;

    for(auto it = clientSockets.begin(); it != clientSockets.end(); ++it){
		std::cout<<"Closing client socket ("<<*it<<")"<<std::endl;

        result = close(*it);
        if(result == -1){
            std::cerr<<"Closing "<<*it<<" socket failed"<<std::endl;
            continue;
        }
    }

    for(auto it = serverSockets.begin(); it != serverSockets.end(); ++it){
		std::cout<<"Closing server socket ("<<*it<<")"<<std::endl;

        result = close(*it);

        if(result == -1){
            std::cerr<<"Closing "<<*it<<" socket failed"<<std::endl;
            continue;
        }
        std::cout<<"losed "<<*it<<" server socket"<<std::endl;
    }
}

void Client::connectTo(const struct sockaddr_in &address)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if(connect(sock, (struct sockaddr *) &address, sizeof address) == -1)         // TODO obsługa błędów - dodać poprawne zamknięcie
        throw std::runtime_error ("connect call failed");

    std::cout << "Connected to sbd" << std::endl;

    clientSockets.push_back(sock);              // TODO obsługa błędów

    maxFd = std::max(maxFd,sock+1);

    if(address.sin_addr.s_addr != server.sin_addr.s_addr)
        clientSocketsNum++;
}

void Client::signal_waiter()
{
	int sig_number;

    sigwait (&signal_set, &sig_number);
    if (sig_number == SIGINT) 
	{
        std::cout<<"\rReceived SIGINT. Exiting..."<<std::endl;
        interrupted_flag = true;
	}
}

void Client::setSigmask(){

    sigemptyset (&signal_set);
    sigaddset (&signal_set, SIGINT);
    int status = pthread_sigmask (SIG_BLOCK, &signal_set, NULL);
    if (status != 0)
        std::cerr<<"Setting signal mask failed"<<std::endl;
}

void Client::handleMessages()
{
    for(auto it=serverSockets.begin(); it!=serverSockets.end();)                // pętla dla serverSockets -> TODO pętla dla cilentSockets
    {
        if(FD_ISSET(*it, &ready) && msg_manager.assembleMsg(*it))
        {
            msg::Message msg = msg_manager.readMsg(*it);

            std::cout << (int) msg.type << std::endl;
            if(msg.type == msg::Message::Type::disconnect_client)                                // tu msg
            {
                close(*it);
                it = serverSockets.erase(it);
                serverSocketsNum--;

                std::cout << "Connection severed" << std::endl;
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

void Client::handleCommands()
{
    commandLock.lock();

    if(command != 0)
    {
        try
        {
            if(command == 99) msg::Message(100,"Hello world!",13).sendMessage(*clientSockets.begin());  //test

            int input_value = command;      // wartość przekazywana do inputHandler w consoleInterface
            command = 0;
            console->handleInput(input_value);          // aby nie zezwolić wątkowi na próbę wypisania menu

            if(console->getState() == State::down)
                endFlag=1;

            condition.notify_one();
        }
        catch(std::exception& e) 
        { 
            std::cerr << e.what() << std::endl; 
            endFlag=1; 
            condition.notify_one(); 
            run_stop_flag=1;
        }
    }

    commandLock.unlock();  
}

void Client::sendFilesInfo()
{
    std::vector<std::string> file_names = std::move(console->getDirFiles());

    for(std::string fname : file_names)
    {
        std::vector<std::string> hashes = hashPieces("clientFiles/" + fname, pieceSize);    //fileDIRname instead of clientFiles
        int i=0;
        
        for(std::string hash : hashes)
        {
            msg::Message fileinfo(310);

            int name_size = fname.size();
            const char* ns = static_cast<char*>( static_cast<void*>(&name_size) );
            fileinfo.buffer.insert(fileinfo.buffer.end(), ns, ns + sizeof(int));    //name size

            fileinfo.buffer.insert(fileinfo.buffer.end(), fname.begin(), fname.end());  //file name

            const char* c = static_cast<char*>( static_cast<void*>(&i) );
            fileinfo.buffer.insert(fileinfo.buffer.end(), c, c + sizeof(int));  //piece number

            fileinfo.buffer.insert(fileinfo.buffer.end(), hash.begin(), hash.end());    //hash for that piece

            fileinfo.buf_length = fileinfo.buffer.size();

            fileinfo.sendMessage(*clientSockets.begin());   //nalezy wyroznic jakos socket przez ktory komunikujemy sie z serwerem

            ++i;
        }
    }
}

void Client::run()
{
    setSigmask();       // Set sigmask for all threads
    signal_thread = std::thread(&Client::signal_waiter, this);        // Create the sigwait thread

    input = std::thread(&Client::input_thread, this);
    sleep(1);
    condition.notify_one(); 

    while(!interrupted_flag && !run_stop_flag && console->getState() != State::down)
    {

        FD_ZERO(&ready);
        FD_SET(sockFd, &ready);

        for(const int &i : clientSockets)
            FD_SET(i, &ready);

        for(const int &i : serverSockets)
            FD_SET(i, &ready); 

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

        handleMessages();
        handleCommands();
    }

	pthread_cancel(signal_thread.native_handle());
    signal_thread.join();
    pthread_cancel(input.native_handle());
    input.join();
    turnOff();
}

const struct sockaddr_in& Client::getServer() const
{
    return server;
}

void Client::setConsoleInterface(ConsoleInterfacePtr& x)
{
    console = std::move(x);
}