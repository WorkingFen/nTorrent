#include "headers/client.hpp"
#include <stdexcept>
#include <string.h>
#include <algorithm>
#include <string>

void Client::input_thread()
{
    std::string input;
    std::unique_lock<std::mutex> lock(inputLock);
    while(!interrupted_flag && console->getState() != State::down)
    {std::cout << "Wait" << std::endl;
        condition.wait(lock);
    std::cout << "Wit not" << std::endl;
        if(console != nullptr)

            {std::cout << "Not nullptr" << std::endl;
                console->printMenu();}
        else
            {std::cout << "Else" << std::endl;
                break;}
        
        std::cin>>input;
        //pthread_mutex_lock(&input_lock);
        commandLock.lock();
        try
        {
            command = std::stoi(input);
        }
        catch(std::invalid_argument e)
        {
            command = 100;
        }
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

Client::Client(const char ipAddr[15], const int& port, const char serverIpAddr[15], const int& serverPort) : clientSocketsNum(0), serverSocketsNum(0), maxFd(0)
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

Client::~Client()
{
    std::cout << "Disconnected" << std::endl;  
}

void Client::connectTo(const struct sockaddr_in &address)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if(connect(sock, (struct sockaddr *) &address, sizeof address) == -1)         // TODO obsługa błędów - dodać poprawne zamknięcie
        throw std::runtime_error ("connect call failed");

    std::cout << "Conected to sbd" << std::endl;

    clientSockets.push_back(sock);              // TODO obsługa błędów

    maxFd = std::max(maxFd,sock+1);

    if(address.sin_addr.s_addr != server.sin_addr.s_addr)
        clientSocketsNum++;

    //sendMessage(sock, msg::Message(100));
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


void Client::run()
{

    setSigmask();       // Set sigmask for all threads

    signal_thread = std::thread(&Client::signal_waiter, this);        // Create the sigwait thread

    //pthread_create(&input, 0, input_thread, 0);
    input = std::thread(&Client::input_thread, this);
    sleep(1);
    condition.notify_one();

    int input_value;        // wartość przekazywana do inputHandler w consoleInterface
    // registerSignalHandler(turnOff);
    while(!interrupted_flag && console->getState() != State::down)
    {

        FD_ZERO(&ready);
        FD_SET(sockFd, &ready);

        for(const int &i : clientSockets)
            FD_SET(i, &ready);

        for(const int &i : serverSockets)
            FD_SET(i, &ready); 

        to.tv_sec = 1;
        to.tv_usec = 0;


        if ( (select(maxFd, &ready, (fd_set *)0, (fd_set *)0, &to)) == -1) {
             throw std::runtime_error ("select call failed");
        }

        //std::cout << maxFd << " " << FD_ISSET(sockFd, &ready) << std::endl;

        if (FD_ISSET(sockFd, &ready) && serverSocketsNum<5)
        {
            int newCommSock = accept(sockFd, (struct sockaddr *)0, (socklen_t *)0);
            serverSockets.push_back(newCommSock);
            serverSocketsNum++;
            maxFd = std::max(maxFd,newCommSock+1);
            std::cout << "serverSocketsNum = " << serverSocketsNum << std::endl;
        }

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
        
        //pthread_mutex_lock(&input_lock);
        commandLock.lock();

        if(command > 0)
        {
            if(command > 0 && command <= 9){
                try{
                    input_value = command;
                    command = 0;
                    console->handleInput(input_value);
                    if(console->getState() != State::down)                  // aby nie zezwolić wątkowi na próbę wypisania menu
                        condition.notify_one();
                }
                catch(std::exception& e) { std::cerr << e.what() << std::endl; break;}
            } else 
            {
                command = 0;
                msg::Message(100,"Hello world!",13).sendMessage(*clientSockets.begin());
                condition.notify_one();
            }
        }

        commandLock.unlock();//pthread_mutex_unlock(&input_lock);
    }
	signal_thread.detach();																			// czekaj aż wątek signal_thread skończy działać				
    input.detach();																								// input blokuje się na std::cin 
    //input.join();
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

/*
void Client::handleMessage(int msg, int src_socket)
{
    switch(msg)
    {
        case 100: disconnectFrom(src_socket); break;
        default: std::cerr << "Unknown message" << std::endl;
    }
}*/

/*
void Client::disconnectFrom(int socket)
{
    auto it = find(serverSockets.begin(), serverSockets.end(), socket);

    if(it == serverSockets.end()) std::cerr << "Closing "<<socket<<" unsuccessful" << std::endl;

    close(*it);
    serverSockets.erase(it);
    //update maxFd ???
}*/


