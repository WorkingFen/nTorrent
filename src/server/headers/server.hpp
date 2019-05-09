#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <list>
#include <stdexcept>
#include <thread>
#include <signal.h>
#include "../../message/message.hpp"
#include "../json/json.h"

// #include <netinet/in.h>

#define SRVNOCONN 404
#define SRVNORM 0
#define SRVERROR -1

#define LOGS

#ifdef _DEBUG
    #define SOCKNAME
    #define CTNAME
#endif

#ifdef LOGS
    #ifndef SOCKNAME
        #define SOCKNAME
    #endif
    #ifndef CTNAME
        #define CTNAME
    #endif
#endif

typedef std::list<int> cts_list;
typedef std::list<int>::iterator cts_list_it; 

namespace server {
    class Server{
        private:    
            msg::MessageManager msg_manager;
            int listener;               // Server-listener socket
            sockaddr_in server;         // Server IP:port
            char msg_buf[4096];         // Buffer for messages
            cts_list clients_sockets;   // List of clients' sockets
            cts_list_it cts_it;         // Iterator of clients' list
            int max_fd;                 // Max file descriptor number
            timeval timeout;            // Timeout for select_ct()
            fd_set bits_fd;             // Bits for file descriptors

            // Json
            // Json::Reader reader;
            // Json::Value root; 
#ifdef CTNAME
            char host[NI_MAXHOST];
            char svc[NI_MAXSERV];
#endif
            std::thread signal_thread;  // Thread for signal handling
            sigset_t signal_set;        // Signal which should be used for set_SIGmask
            bool sigint_flag;           // Signal of ctrl+C usage
            void signal_waiter(); 
            void set_SIGmask();
            void shutdown();

        public:
            Server(const char srv_ip[15], const int& srv_port);
            ~Server();

            void socket_srv();
            void bind_srv(const char srv_ip[15], const int& srv_port);
            void listen_srv();
            void select_cts();
            void accept_srv();
            void check_cts();
            int write_srv(const void* buffer, size_t msg_size);
            int read_srv(char* buffer);
            void close_ct();
    };
}

#endif