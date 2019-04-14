#include "headers/client.hpp"
#include "headers/message.hpp"
#include <stdexcept>
#include <string.h>
#include <algorithm>


Client::Client(const char ipAddr[15], const int& port) : clientSocketsNum(0), serverSocketsNum(0), maxFd(0), state(State::down)
{
    self.sin_family = AF_INET;

    if( (inet_pton(AF_INET, ipAddr, &self.sin_addr)) == -1)
        throw std::invalid_argument("Improper IP address");
    
    self.sin_port = htons(port);

    if( (sockFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        throw std::runtime_error ("socket call failed");

    maxFd = sockFd+1;

    if(bind(sockFd, (struct sockaddr *) &self, sizeof self) == -1)
        throw std::runtime_error ("bind call failed");

    if(listen(sockFd, 20) == -1)
        throw std::runtime_error ("listen call failed");

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
    
    state = State::up;

    std::cout << sockFd << std::endl;
    std::cout << "Successfully connected and listening at: " << ipAddr << ":" << this->port << std::endl;
}

void Client::turnOff()
{
    
}

Client::~Client()
{
    close(sockFd);
    std::cout << "Disconnected" << std::endl;  
}

void Client::connectTo(struct sockaddr_in &server)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    // obsługa błędów - dodać poprawne zamknięcie
    if(connect(sock, (struct sockaddr *) &server, sizeof server) == -1)
        throw std::runtime_error ("connect call failed");

    std::cout << "Conected to sbd" << std::endl;
    // obsługa błędów
    clientSockets.push_back(sock);

    maxFd = std::max(maxFd,sock+1);

    // sprawdzenie czy nie serwer i inkrementacja oSockets
}

void Client::run()
{
    do{
        FD_ZERO(&ready);
        FD_SET(sockFd, &ready);

        for(const int &i : clientSockets)
            FD_SET(i, &ready);

        for(const int &i : serverSockets)
            FD_SET(i, &ready);

        to.tv_sec = 1;                          /// przenieść póżniej do konstruktora
        to.tv_usec = 0;

        if ( (select(maxFd, &ready, (fd_set *)0, (fd_set *)0, &to)) == -1) {
             throw std::runtime_error ("select call failed");
        }

        std::cout << maxFd << " " << FD_ISSET(sockFd, &ready) << std::endl;

        if (FD_ISSET(sockFd, &ready) && serverSocketsNum<5)
        {
            int newCommSock = accept(sockFd, (struct sockaddr *)0, (socklen_t *)0);
            serverSockets.push_back(newCommSock);                             // jak rozróżniać iSockets od oSockets; możliwe, że trzeba będzie rozbić commSockets na 2 listy
            serverSocketsNum++;
            maxFd = std::max(maxFd,newCommSock+1);
            std::cout << "serverSocketsNum = " << serverSocketsNum << std::endl;
        }

        for(auto it=serverSockets.begin(); it!=serverSockets.end();)                // pętla dla serverSockets -> TODO pętla dla cilentSockets
        {
            msg::Message msg = msg::readMessage(*it);

            if(msg.type == msg::Message::Type::disconnect_client)                                // tu msg
            {
                it = serverSockets.erase(it);
                serverSocketsNum--;
                close(*it);

                std::cout << "Connection severed" << std::endl;
            }
            else
            {
                ++it;
            }
        }

    }while(true);
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

    if(it == serverSockets.end()) std::cerr << "disconnect unsuccessful" << std::endl;

    close(*it);
    serverSockets.erase(it);
    //update maxFd ???
}*/