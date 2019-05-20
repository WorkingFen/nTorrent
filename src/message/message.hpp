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
        share_file,
        list_files,
        delete_block,
        ask_for_file,
        have_block,
        ask_for_block,
        bad_block_hash,
        keep_alive,
        client_disconnected,

        file_info,
        block_info,
        server_out,

        file_block,

        get_file,

        broken,
    };

    static const std::map<int, Type> type_map;

    int type = -1;
    int buf_length = 0;
    std::vector<char> buffer;

    Type getType(int t) const;

    int sendMessage(int dst_socket);

    int readInt();
    std::string readString(int length);
    void writeInt(int i);
    void writeString(std::string s);

    Message() {}
    Message(int t): type(t) {}

    Message(int t, char* b, int bl): type(t), buf_length(bl)
    {
        buffer = std::vector<char>(b, b+bl);
    }
};

class MessageManager
{
    private:
        std::map<int, std::vector<char>> buffers;
        std::map<int, Message> msg_buffer;

        int read_result;

        bool isMsgHeaderReady(int socket);
        //verify msg type after header has been received
        size_t expectedMsgSize(int socket);
        size_t remainingMsgSize(int socket);

        int popIntFromBuffer(int socket);

    public:
        int lastReadResult() const;
        bool assembleMsg(int socket);

        //size_t countMsg(int socket);
        Message readMsg(int socket);
};

}
#endif