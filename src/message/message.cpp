#include "message.hpp"
#include <iostream>

using namespace msg;

int MessageManager::lastReadResult() const
{
    return read_result;
}

int Message::sendMessage(int dst_socket)
{std::cout << "Message sent" << std::endl;
    if(write(dst_socket, &buf_length, sizeof(buf_length)) < 0)
    {
        std::cerr << "Unsuccessful write (body length)" << std::endl;
        return -1;
    };

    if(write(dst_socket, &type, sizeof(type)) < 0)
    {
        std::cerr << "Unsuccessful write (msg type)" << std::endl;
        return -1;
    };

    if(write(dst_socket, &buffer[0], buf_length) < 0)
    {
        std::cerr << "Unsuccessful write (body)" << std::endl;
        return -1;
    };

    return buf_length + 2*sizeof(int);
}
/*
int Message::readMessage(int socketFd, Message &message)
{
    Message msg;
    int bytesRead = 0;
    int length = 4;
    char buf_msg[4];

    while(bytesRead < length)
    {
        int readReturn = read(socketFd, buf_msg, length);
        if(readReturn < 1) return -1;//throw std::runtime_error("Unsuccessful read");
        bytesRead += readReturn;
    }

    msg.type = static_cast<Message::Type>((buf_msg[3] << 24) | (buf_msg[2] << 16) | (buf_msg[1] << 8) | (buf_msg[0]));

    char buf_length[4];
    bytesRead = 0;

    while(bytesRead < length)
    {
        int readReturn = read(socketFd, buf_length, length);
        if(readReturn < 1) return -1;//throw std::runtime_error("Unsuccessful read");
        bytesRead += readReturn;
    }

    msg.buf_length = (buf_length[3] << 24) | (buf_length[2] << 16) | (buf_length[1] << 8) | (buf_length[0]);

    std::vector<char> buffer(msg.buf_length);
    bytesRead = 0;

    while(bytesRead < msg.buf_length)
    {
        int readReturn = read(socketFd, &buffer[0], msg.buf_length);
        if(readReturn < 1) return -1;//throw std::runtime_error("Unsuccessful read");
        bytesRead += readReturn;
    }

    msg.buffer = buffer;

    message = msg;
    return message.buf_length + sizeof(message.type) + sizeof(message.buf_length);
}
*/
int MessageManager::popIntFromBuffer(int socket)
{
    int ret = (buffers[socket][3] << 24) | (buffers[socket][2] << 16) | (buffers[socket][1] << 8) | (buffers[socket][0]);
    buffers[socket].erase(buffers[socket].begin(), buffers[socket].begin() + 4);

    return ret;
}

bool MessageManager::isMsgHeaderReady(int socket)
{
    return buffers[socket].size() >= 2*sizeof(int);
}

size_t MessageManager::expectedMsgSize(int socket)
{
    if(!isMsgHeaderReady(socket)) return 2*sizeof(int);
    return 2*sizeof(int) + ((buffers[socket][3] << 24) | (buffers[socket][2] << 16) | (buffers[socket][1] << 8) | (buffers[socket][0])) ;
}

size_t MessageManager::remainingMsgSize(int socket)
{
    return expectedMsgSize(socket) - buffers[socket].size();
}

bool MessageManager::assembleMsg(int socket)
{
    if(!isMsgHeaderReady(socket)) 
    {
        std::vector<char> buffer(8-buffers[socket].size());

        read_result = read(socket, &buffer[0], 8-buffers[socket].size());
        if(read_result == -1) return false;
        buffers[socket].insert(buffers[socket].end(), buffer.begin(), buffer.begin() + read_result);

        if(!isMsgHeaderReady(socket)) return false;
    }

    std::vector<char> buffer(remainingMsgSize(socket));

    read_result = read(socket, &buffer[0], remainingMsgSize(socket));
    buffers[socket].insert(buffers[socket].end(), buffer.begin(), buffer.end());

    if(remainingMsgSize(socket) > 0) return false;

    Message msg;
    msg.buf_length = popIntFromBuffer(socket);
    msg.type = static_cast<Message::Type>(popIntFromBuffer(socket));
    msg.buffer = std::vector<char>(buffers[socket].begin(), buffers[socket].begin() + msg.buf_length);
    //msg.buffer = buffers[socket];//std::vector<char>(buffers[socket].begin(), buffers[socket].begin() + msg.buf_length);

    //buffers[socket].erase(buffers[socket].begin(), buffers[socket].begin() + msg.buf_length);
    buffers[socket].clear();

    msg_buffer.insert({socket, msg});

    return true;
}

//size_t MessageManager::countMsg(int socket);
Message MessageManager::readMsg(int socket)
{
    Message msg = msg_buffer[socket];
    msg_buffer.erase(socket);
    return msg;
}

/*
13  0   0   0   100 0 0 0 72 101 108 108 111 32 119 111 114 108 100 33 0 

13  0   0   0   0   0   0   0   100 0 0 0 72 101 108 108 111 32 119 111 114 
 */