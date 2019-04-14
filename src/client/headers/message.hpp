#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
/*
class Message{
    public:
    enum class Type
    {
        disconnect_client,
        keep_alive,

        broken,
    };

    private:
    Type msg_type;

    public:
    Message(Type type_): msg_type(type_) {}
    Type getType() { return msg_type; }
};
*/




/*
    disconnect_client = 100
    keep_alive = ???

    broken = -1
*/

int sendMessage(int msg, int dst_socket)
{
    if(write(dst_socket, &msg, sizeof(msg)) < 0)
    {
        std::cerr << "Unsuccessful write" << std::endl;
        return -1;
    };

    return msg;
}

int readMessage(int socketFd)
{
    int bytesRead = 0;
    int length = 4;
    char buffer[4];

    while(bytesRead < length)
    {
        int readReturn = read(socketFd, buffer, length);

        if(readReturn < 1) return -1;//throw std::runtime_error("Unsuccessful read");

        bytesRead += readReturn;
    }

    return (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | (buffer[0]);
}

#endif