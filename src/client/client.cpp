#include "headers/client.hpp"
#include <stdexcept>

Client::Client(const char ipAddr[15], const int& port) : iSockets(0), oSockets(0)
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

    std::cout << "Successfully connected and listening at: " << ipAddr << ":" << ntohs(self.sin_port) << std::endl;
}

Client::~Client()
{
    close(sockFd);
    std::cout << "Disconnected" << std::endl;
}

void Client::connectTo(struct sockaddr_in &server)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    // obsługa błędów
    connect(sock, (struct sockaddr *) &server, sizeof server);
    // obsługa błędów
    commSockets.push_back(sock);

    // sprawdzenie czy nie serwer i inkrementacja oSockets
}