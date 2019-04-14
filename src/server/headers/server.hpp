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

// #include <netinet/in.h>
// #include <stdexcept>

#define SRVNORM 0
#define SRVERROR -1

#ifdef _DEBUG
    #define SOCKNAME
    #define CTNAME
#endif

#ifdef LOGS
    #ifndef CTNAME
        #define CTNAME
    #endif
#endif

typedef std::list<int> cts_list;
typedef std::list<int>::iterator cts_list_it; 

namespace server {
    class Server{
        private:    
            int listener;
            sockaddr_in server;
            char msg_buf[4096];
            cts_list clients_sockets;
            cts_list_it cts_it;
            sockaddr_in client;
            socklen_t client_size;
#ifdef CTNAME
            char host[NI_MAXHOST];
            char svc[NI_MAXSERV];
#endif

        public:
            Server(const char srv_ip[15], const int& srv_port);
            ~Server();

            int socket_srv();
            int bind_srv(const char srv_ip[15], const int& srv_port);
            int listen_srv();
            int accept_srv();
            int write_srv(const void* buffer, size_t msg_size);
            int read_srv(char* buffer);

            int close_ct();

            //void close();

    };
}

#endif