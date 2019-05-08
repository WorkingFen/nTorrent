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
#include <map>

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

    int sendMessage(int dst_socket);

    Message() {}
    Message(Type t): type(t) {}
    Message(int t): type(static_cast<Type>(t)) {}

    Message(int t, char* b, int bl): type(static_cast<Type>(t)), buf_length(bl)
    {
        buffer = std::vector<char>(b, b+bl);
    }

    Message(Type t, char* b, int bl): type(t), buf_length(bl)
    {
        buffer = std::vector<char>(b, b+bl);
    }
};

class MessageManager
{
    private:
        std::map<int, std::vector<char>> buffers;
        std::map<int, Message> msg_buffer;

        bool isMsgHeaderReady(int socket);
        size_t expectedMsgSize(int socket);
        size_t remainingMsgSize(int socket);

        int popIntFromBuffer(int socket);

    public:
        bool assembleMsg(int socket);

        //size_t countMsg(int socket);
        Message readMsg(int socket);
};

}
#endif