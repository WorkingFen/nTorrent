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
#include <vector>

// #include <netinet/in.h>
// #include <list>
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

namespace server {
    class Server{
        private:    
            int listener;
            sockaddr_in server;
            char msg_buf[4096];
            std::vector<int> client_socket;
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
            int write_srv(int client_id, const void* buffer, size_t msg_size);
            int read_srv(int client_id, char* buffer);

            int close_ct(int client_id);

            //void close();

    };
}

#endif