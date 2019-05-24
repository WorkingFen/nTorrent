#include "message.hpp"
#include <iostream>
#include <cstring>

using namespace msg;

const std::map<int, Message::Type> Message::type_map =
{
    {101, Message::Type::share_file},
    {102, Message::Type::list_files},
    {103, Message::Type::delete_block},
    {104, Message::Type::ask_for_file},
    {105, Message::Type::have_block},
    {106, Message::Type::ask_for_block},
    {107, Message::Type::bad_block_hash},
    {110, Message::Type::keep_alive},
    {111, Message::Type::client_disconnected},

    {201, Message::Type::file_info},
    {202, Message::Type::block_info},
    {209, Message::Type::srv_keepalive},
    {210, Message::Type::srv_hello},
    {211, Message::Type::server_out},

    {301, Message::Type::get_file},
    //{302, Message::Type::stop_seeding},

    {401, Message::Type::file_block},
};

Message::Type Message::getType(int t) const
{
    auto message = type_map.find(t);
    if(message == type_map.end()) return Message::Type::broken;

    return message->second;
}

int Message::readInt()
{
    //int ret = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | (buffer[0]);
    int ret;
    std::memcpy(&ret, &buffer[0], sizeof(int));
    buffer.erase(buffer.begin(), buffer.begin() + sizeof(int));

    buf_length -= sizeof(int);

    return ret;
}

std::string Message::readString(int length)
{
    std::string ret(buffer.begin(), buffer.begin() + length);
    buffer.erase(buffer.begin(), buffer.begin() + length);

    buf_length -= length;

    return ret;
}

void Message::writeInt(int i)
{
    const char* ns = static_cast<char*>( static_cast<void*>(&i) );

    buffer.insert(buffer.end(), ns, ns + sizeof(int));    

    buf_length += sizeof(int);
}

void Message::writeString(std::string s)
{
    buffer.insert(buffer.end(), s.begin(), s.end());

    buf_length += s.length();
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

int MessageManager::lastReadResult() const
{
    return read_result;
}

int MessageManager::popIntFromBuffer(int socket)
{
    //int ret = (buffers[socket][3] << 24) | (buffers[socket][2] << 16) | (buffers[socket][1] << 8) | (buffers[socket][0]);
    int ret;
    std::memcpy(&ret, &buffers[socket][0], sizeof(int));   
    
    buffers[socket].erase(buffers[socket].begin(), buffers[socket].begin() + sizeof(int));

    return ret;
}

bool MessageManager::isMsgHeaderReady(int socket)
{
    return buffers[socket].size() >= 2*sizeof(int);
}

size_t MessageManager::expectedMsgSize(int socket)
{
    if(!isMsgHeaderReady(socket)) return 2*sizeof(int);
    
    int len;
    std::memcpy(&len, &buffers[socket][0], sizeof(int)); 
    return 2*sizeof(int) + len;//((buffers[socket][3] << 24) | (buffers[socket][2] << 16) | (buffers[socket][1] << 8) | (buffers[socket][0])) ;
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
    if(read_result == -1) return false;
    buffers[socket].insert(buffers[socket].end(), buffer.begin(), buffer.end());

    if(remainingMsgSize(socket) > 0) return false;

    Message msg;
    msg.buf_length = popIntFromBuffer(socket);
    msg.type = popIntFromBuffer(socket);
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