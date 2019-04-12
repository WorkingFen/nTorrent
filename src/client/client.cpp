#include "headers/client.hpp"
#include <stdexcept>
#include <string.h>

Client::Client(const char ipAddr[15], const int& port) : iSockets(0), oSockets(0), state(State::down)
{
    self.sin_family = AF_INET;

    if( (inet_pton(AF_INET, ipAddr, &self.sin_addr)) == -1)
        throw std::invalid_argument("Improper IP address");
    
    self.sin_port = htons(port);

    if( (sockFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        throw std::runtime_error ("socket call failed");

    if(bind(sockFd, (struct sockaddr *) &self, sizeof self) == -1)
        throw std::runtime_error ("bind call failed");

    if(listen(sockFd, 20) == -1)
        throw std::runtime_error ("listen call failed");

    getcwd(fileDir, 1000);
    strcat(fileDir, "/clientFiles");

    struct sockaddr_in x;
    socklen_t y = sizeof(self);
    getsockname(sockFd, (struct sockaddr *) &x, &y);

    this->port = (int) ntohs(x.sin_port);
    state = State::up;

    std::cout << sockFd << std::endl;
    std::cout << "Successfully connected and listening at: " << ipAddr << ":" << this->port << std::endl;
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
    commSockets.push_back(sock);

    // sprawdzenie czy nie serwer i inkrementacja oSockets
}

void Client::run()
{
    int numOfDescr;
    do{
        numOfDescr=1;
        FD_ZERO(&ready);
        FD_SET(sockFd, &ready);

        for(const int &i : commSockets)
        {
            FD_SET(i, &ready);
            numOfDescr++;
        }

        to.tv_sec = 5;                          /// przenieść póżniej do konstruktora
        to.tv_usec = 0;

        if ( (select(numOfDescr-1, &ready, (fd_set *)0, (fd_set *)0, &to)) == -1) {
             throw std::runtime_error ("select call failed");
        }

        if (FD_ISSET(sockFd, &ready) && oSockets<5)
        {
            int newCommSock = accept(sockFd, (struct sockaddr *)0, (socklen_t *)0);
            std::cout << "Accepted sbd" << std::endl;
            commSockets.push_back(newCommSock);                             // jak rozróżniać iSockets od oSockets; możliwe, że trzeba będzie rozbić commSockets na 2 listy
            oSockets++;
        }

    }while(true);
}