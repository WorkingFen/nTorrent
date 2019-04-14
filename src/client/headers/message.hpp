#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>

namespace msg
{

struct Message
{
    enum class Type : int
    {
        disconnect_client = 100,
        keep_alive = 210,

        broken = -1,
    };

    Type type = Type::broken;
    int buf_length = 0;
    std::vector<char> buffer;

    Message() {}
    Message(Type t): type(t) {}
    Message(int t): type(static_cast<Type>(t)) {}

    Message(Type t, char* b, int bl): type(t), buf_length(bl)
    {
        buffer = std::vector<char>(b, b+bl);
    }
};

int sendMessage(int dst_socket, Message &message)
{
    if(write(dst_socket, &message.type, sizeof(message.type)) < 0)
    {
        std::cerr << "Unsuccessful write (msg type)" << std::endl;
        return -1;
    };

    if(write(dst_socket, &message.buf_length, sizeof(message.buf_length)) < 0)
    {
        std::cerr << "Unsuccessful write (body length)" << std::endl;
        return -1;
    };

    if(write(dst_socket, &message.buffer[0], message.buf_length) < 0)
    {
        std::cerr << "Unsuccessful write (body)" << std::endl;
        return -1;
    };

    return static_cast<int>(message.type);
}

Message readMessage(int socketFd)
{
    Message msg;
    int bytesRead = 0;
    int length = 4;
    char buf_msg[4];

    while(bytesRead < length)
    {
        int readReturn = read(socketFd, buf_msg, length);
        if(readReturn < 1) return Message(-1);//throw std::runtime_error("Unsuccessful read");
        bytesRead += readReturn;
    }

    msg.type = static_cast<Message::Type>((buf_msg[3] << 24) | (buf_msg[2] << 16) | (buf_msg[1] << 8) | (buf_msg[0]));

    char buf_length[4];
    bytesRead = 0;

    while(bytesRead < length)
    {
        int readReturn = read(socketFd, buf_length, length);
        if(readReturn < 1) return Message(-1);//throw std::runtime_error("Unsuccessful read");
        bytesRead += readReturn;
    }

    msg.buf_length = (buf_length[3] << 24) | (buf_length[2] << 16) | (buf_length[1] << 8) | (buf_length[0]);

    std::vector<char> buffer(msg.buf_length);
    bytesRead = 0;

    while(bytesRead < msg.buf_length)
    {
        int readReturn = read(socketFd, &buffer[0], msg.buf_length);
        if(readReturn < 1) return Message(-1);//throw std::runtime_error("Unsuccessful read");
        bytesRead += readReturn;
    }

    msg.buffer = buffer;

    return msg;
}

}
#endif