#include "headers/server.hpp"

server::Server::Server(const char srv_ip[15], const int& srv_port) {
    socket_srv();
    bind_srv(srv_ip, srv_port);
    listen_srv();
    while(true){
        accept_srv();
        close_ct(client_socket.size()-1);
    }
}

// Create listener socket
int server::Server::socket_srv() {
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener == -1) {
        std::cerr << "Can't create a socket!";
        return SRVERROR;
    }
    return SRVNORM;
}

// Bind socket to ip:port 
int server::Server::bind_srv(const char srv_ip[15], const int& srv_port) {
    server.sin_family = AF_INET;
    server.sin_port = htons(srv_port);
    inet_pton(AF_INET, srv_ip, &server.sin_addr);

    if(bind(listener, (sockaddr*)&server, sizeof(server)) == -1) {
        std::cerr << "Can't bind to IP:port";
        return SRVERROR;
    }

#ifdef SOCKNAME
    if(getsockname(listener, (sockaddr*)&server, sizeof(server)) == -1) {
        std::cerr << "Can't get socket name";
        return SRVERROR;
    }
    std::cout << "Socket port #" << ntohs(server.sin_port) << std::endl;
#endif

    return SRVNORM;
}

// Start listening on listener socket, with SOMAXCONN connections
int server::Server::listen_srv() {
    if(listen(listener, SOMAXCONN) == -1) {
        std::cerr << "Can't listen!";
        return SRVERROR;
    }
    return SRVNORM;
}

// Accept new client
int server::Server::accept_srv() {
    client_size = sizeof(client);
    client_socket.push_back(accept(listener, (sockaddr*)&client, &client_size));

    if(client_socket.back() == -1) {
        std::cerr << "Problem with client connecting!";
        return SRVERROR;
    }
    else while(true) {
#ifdef CTNAME
        memset(host, 0, NI_MAXHOST);
        memset(svc, 0, NI_MAXSERV);
        int result = getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, svc, NI_MAXSERV, 0);

        if(result) {
            std::cout << host << " connected on " << svc << std::endl;
        }
        else {
            inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
            std::cout << host << " connected on " << ntohs(client.sin_port) << std::endl;
        }
#endif
        int bytes_rcv = read_srv(client_socket.size()-1, msg_buf);
        if(bytes_rcv == SRVERROR) return SRVERROR;
        else if(bytes_rcv == SRVNORM) return SRVNORM;
        write_srv(client_socket.size()-1, msg_buf, bytes_rcv+1);
    }
}

// Write new message to client with ID: client_id
int server::Server::write_srv(int client_id, const void* buffer, size_t msg_size) {
    if(send(client_socket[client_id], buffer, msg_size, 0) == -1) return SRVERROR;
    else return SRVNORM;
}

// Read message from client with ID: client_id
int server::Server::read_srv(int client_id, char* buffer) {
    memset(buffer, 0, sizeof(buffer));

    int bytes_rcv = recv(client_socket[client_id], buffer, sizeof(buffer), 0);
    
    if(bytes_rcv == -1) {
        std::cerr << "There was a connection issue" << std::endl;
        return SRVERROR;
    }
    if(bytes_rcv == 0) {
        std::cout << "The client disconnected" << std::endl;
        return SRVNORM;
    }

    // Display message
    std::cout << "Received: " << std::string(buffer, 0, bytes_rcv) << std::endl;
    return bytes_rcv;
}

// Close connection with client whose ID: client_id
int server::Server::close_ct(int client_id) {
    if(close(client_socket[client_id]) == -1) return SRVERROR;
    else return SRVNORM;
}

server::Server::~Server() {
#ifdef CTNAME
    memset(host, 0, NI_MAXHOST);
    memset(svc, 0, NI_MAXSERV);
#endif
    memset(msg_buf, 0, sizeof(msg_buf));
    close(listener);
    for(int i = 0; i < client_socket.size(); i++)
        close_ct(i);
    std::cout << "Disconnected" << std::endl;
}