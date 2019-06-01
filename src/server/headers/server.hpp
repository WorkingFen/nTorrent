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

#define SRV_NOCONN 404
#define SRV_OK 1
#define SRV_ERROR -1

#define MSG_OK 1
#define MSG_RNGERROR 0
#define MSG_ERROR -1

#define LOGS
#define COLORCONSOLE
#define FILES
#define OWNED

#ifdef _DEBUG
    #ifndef LOGS 
        #define LOGS
    #endif
    #ifndef MSGINFO
        #define MSGINFO
    #endif
    #ifndef FILES
        #define FILES
    #endif
#endif

#ifdef LOGS
    #ifndef SRVNAME
        #define SRVNAME
    #endif
    #ifndef CTNAME
        #define CTNAME
    #endif
    #ifndef ERROR
        #define ERROR
    #endif
#endif

#ifdef MSGINFO
    #ifdef ISALIVE
        #define KEEPALIVE
    #endif
    #ifdef ADVANCED
        #define MSGADV
    #endif
#endif

#ifdef FILES
    #ifdef OWNED
        #define OWNEDFILES
    #endif
    #ifdef ADVANCED
        #define FILESADV
    #endif
#endif

#ifdef COLORCONSOLE
    #define BOLDF "\u001B[1m"                       // Used for important informations
    #define REDF "\u001B[31m"                       // Used for errors or SIGINT program execution
    #define BGREENF "\u001B[38;5;2m"                // Used for message informations
    #define SKYF "\u001B[38;5;39m"                  // Used for server informations
    #define MINTF "\u001B[38;5;42m"                 // Used for ip addresses and ports
    #define GOLDF "\u001B[38;5;178m"                // Used for client informations
    #define BREDF "\u001B[38;5;196m"                // Used for warnings or disconnection
    #define ORANGEF "\u001B[38;5;209m"              // Used for file informations
    #define RESETF "\u001B[0m"                      // Reset console font
#else 
    #define BOLDF ""
    #define REDF ""
    #define BGREENF ""
    #define SKYF ""
    #define MINTF ""
    #define GOLDF ""
    #define BREDF ""
    #define ORANGEF ""
    #define RESETF ""
#endif

namespace server {
typedef std::chrono::high_resolution_clock::time_point t_point;
typedef std::map<std::string, std::map<int,int>> owned;
typedef std::map<std::string, std::map<int, sockaddr_in>> download;

    struct client {
        sockaddr_in address;                        // Client IP:port
        sockaddr_in call_addr;                      // IP:port on which client is listening
        int socket;                                 // Client's socket
        int no_leeches;                             // Number of active leeches
        t_point timeout;                            // Time of last connection check
        owned o_files;                              // Files owned by client
        download d_files;                           // Files that are downloaded by client

        client() {}
        client(sockaddr_in addr, int s, int nl = 0, t_point t = std::chrono::high_resolution_clock::now()): 
            address(addr), socket(s), no_leeches(nl), timeout(t) {}
    };

typedef std::list<client> cts_list;
typedef std::list<client>::iterator cts_list_it;

    struct block {
        bool empty;
        std::string hash;                           // Block hash
        std::vector<client*> owners;                // Vector of clients who do have this block

        block() : empty(true) {}
        block(std::string h): empty(false), hash(h) {}
    };
    
    struct file {
        std::string name;                           // Name of file
        int size;                                   // Size of file
        std::vector<block> blocks;                  // Vector of blocks

        file() {}
        file(std::string n, int s): name(n), size(s) {}
    };

    class Server {
        private:
            static const int pieceSize = 100000;

            sockaddr_in server;                     // Server IP:port
            int listener;                           // Server-listener socket

            cts_list clients;                       // List of clients
            cts_list_it cts_it;                     // Iterator of clients' list

            fd_set bits_fd;                         // Bits for file descriptors
            int max_fd;                             // Max file descriptor number

            timeval timeout;                        // Timeout for select_ct()

            std::chrono::seconds timesup;           // Time after which client should be disconnected
            
            std::map<std::string, file> files;      // Map of files
            msg::MessageManager msg_manager;
#ifdef CTNAME
            char host[NI_MAXHOST];
            char svc[NI_MAXSERV];
#endif
            std::thread signal_thread;              // Thread for signal handling
            sigset_t signal_set;                    // Signal which should be used for set_SIGmask
            bool sigint_flag;                       // Signal of ctrl+C usage
            void signal_waiter(); 
            void set_SIGmask();
            void shutdown();

        public:
            Server(const char srv_ip[15], const int& srv_port);
            ~Server();

            // Log functions
            void show_ct_ofiles();
            void show_ct_dfiles(); 
            void show_srvfiles(file&);

            file* add_file(int, std::string);
            block* add_block(file&, std::string, uint);
            int add_block_owner(file&, std::string, uint);
            void add_owner(block&, client*);

            file* get_file(std::string);
            block* get_block(file&, uint);

            void delete_file(file&);
            bool delete_block(file&, int);
            bool delete_owner(block&);
            bool delete_owner(block&, uint, int);

            std::pair<client*, int> find_least_occupied(file&, std::vector<int>);

            bool still_alive();
            void set_leeches(std::pair<server::client*, int>&, std::string);
            void set_leeches(std::string, int, uint, int);
            void be_owned(std::string, int);
            void not_owned(std::string, int);
            void not_owned(std::string, int, uint, int);
            void be_downloaded(std::string, int, uint, int);
            void release_downloaded(std::string, int, uint, int);

            void run_srv();
            void close_srv();
            void socket_srv();
            void bind_srv(const char srv_ip[15], const int& srv_port);
            void listen_srv();
            void select_cts();
            void accept_srv();
            void check_cts();
            int read_srv();
            void close_ct();
    };
}

#endif