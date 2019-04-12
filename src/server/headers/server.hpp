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

// #include <netinet/in.h>
// #include <list>
// #include <stdexcept>

#define SRVERROR -1
#define SRVNORM 0

#ifdef _DEBUG
    #define SOCKNAME
    #define CTNAME
#endif

#ifdef LOGS
    #ifndef CTNAME
    #define CTNAME
    #endif
#endif

namespace server {
    class Server{
        private:    
            int listener; // Socket descriptor for server's listener
            sockaddr_in server;
            char msg_buf[4096];
            int client_socket;
            sockaddr_in client;
            socklen_t client_size;
            char host[NI_MAXHOST];
            char svc[NI_MAXSERV];

        public:
            Server(const int& srv_port); // tworzy socketa, który będzie nasłuchiwał
            ~Server();

            int socket_srv();
            int bind_srv(const int& srv_port);
            int listen_srv();
            int accept_srv();
            int write_srv(const void* buffer, size_t msg_size);
            int read_srv(char* buffer);

            int close_ct();

            //void close();

    };
}

#endif