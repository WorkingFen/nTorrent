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
#include <chrono>
#include "../../message/message.hpp"

#define SRVNOCONN 404
#define SRVNORM 1
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

namespace server {
    struct client {
        sockaddr_in address;            // Client IP:port
        int socket;                     // Client's socket
        int no_leeches;                 // Number of active leeches
        int timeout;                    // Time after which connection should end   // ?

        client() {}
        client(sockaddr_in addr, int s, int t = 0, int nl = 0): 
            address(addr), socket(s), no_leeches(nl), timeout(t) {}
    };

typedef std::list<client> cts_list;
typedef std::list<client>::iterator cts_list_it;

    struct block {
        std::string hash;               // Block hash
        cts_list owners;                // Vector of clients who do have this block

        block() {}
        block(std::string h): hash(h) {}
    };
    
    struct file {
        std::string name;               // Name of file
        int size;                       // Size of file
        std::vector<block> blocks;      // Vector of blocks

        file() {}
        file(std::string n, int s): name(n), size(s) {}
    }; 

    class Server{
        private:    
            sockaddr_in server;         // Server IP:port
            int listener;               // Server-listener socket

            cts_list clients;   // List of clients
            cts_list_it cts_it; // Iterator of clients' list

            fd_set bits_fd;             // Bits for file descriptors
            int max_fd;                 // Max file descriptor number

            timeval timeout;            // Timeout for select_ct()          // ?
            
            std::vector<file> files;    // Vector of files
            msg::MessageManager msg_manager;
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

            file add_file(std::string, int);
            block add_block(file&, int, std::string);
            void add_owner(block&, client);

            file get_file(std::string);
            block get_block(std::string);
            block get_block(int);
            client get_owner(sockaddr_in);
            client get_owner(int);

            void delete_file();
            void delete_block();
            void delete_owner();

            void socket_srv();
            void bind_srv(const char srv_ip[15], const int& srv_port);
            void listen_srv();
            void select_cts();
            void accept_srv();
            void check_cts();
            int write_srv(const void* buffer, size_t msg_size);
            int read_srv();
            void close_ct();
    };
}

#endif