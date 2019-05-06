#include "../client/headers/message.hpp"
#include "headers/server.hpp"

// Wait for SIGINT to occur
void server::Server::signal_waiter() {
    int sig;

    sigwait(&signal_set, &sig);
    if(sig == SIGINT) {
        std::cout << "\rReceived SIGINT. Exiting..." << std::endl;
        sigint_flag = true;
    }
}

// Set signal_set to SIGINT
void server::Server::set_SIGmask() {
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGINT);
    int status = pthread_sigmask(SIG_BLOCK, &signal_set, NULL);
    if(status != 0)
        throw std::runtime_error("Setting signal mask failed");
        //std::cerr << "Setting signal mask failed" << std::endl;
}

// Close all connections
void server::Server::shutdown() {
#ifdef CTNAME
    memset(host, 0, NI_MAXHOST);
    memset(svc, 0, NI_MAXSERV);
#endif
    memset(msg_buf, 0, sizeof(msg_buf));
    while(clients_sockets.size() != 0) {
        cts_it = clients_sockets.begin();
        close_ct();
    }
    close(listener);
}

server::Server::Server(const char srv_ip[15], const int& srv_port) : sigint_flag(false) {
    set_SIGmask();
    signal_thread = std::thread(&Server::signal_waiter, this);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    socket_srv();
    bind_srv(srv_ip, srv_port);
    listen_srv();
    while(!sigint_flag){
        select_cts();
        accept_srv();
        check_cts();
    }
    signal_thread.detach();
    shutdown();
}

// Create listener socket
void server::Server::socket_srv() {
    if((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        throw std::runtime_error("Can't create a socket!");
        //std::cerr << "Can't create a socket";
        //return SRVERROR;
    }
    max_fd = listener + 1;
    //return SRVNORM;
}

// Bind socket to ip:port 
void server::Server::bind_srv(const char srv_ip[15], const int& srv_port) {
    server.sin_family = AF_INET;
    server.sin_port = htons(srv_port);
    if(inet_pton(AF_INET, srv_ip, &server.sin_addr) == -1) {
        throw std::domain_error("Improper IPv4 address");
        //std::cerr << "Improper IPv4 address";
        //return SRVERROR;
    }

    if(bind(listener, (sockaddr*)&server, sizeof(server)) == -1) {
        throw std::runtime_error("Can't bind to IP:port");
        //std::cerr << "Can't bind to IP:port";
        //return SRVERROR;
    }

#ifdef SOCKNAME
    socklen_t sa_len = sizeof(server);
    if(getsockname(listener, (sockaddr*)&server, &sa_len) == -1) {
        throw std::runtime_error("Can't get socket name");
        //std::cerr << "Can't get socket name";
        //return SRVERROR;
    }
    std::cout << "Successfully connected and listening at: " << srv_ip << ":" << ntohs(server.sin_port) << std::endl;
#endif
    
    //return SRVNORM;
}

// Start listening on listener socket, with SOMAXCONN connections
void server::Server::listen_srv() {
    if(listen(listener, SOMAXCONN) == -1) {
        throw std::runtime_error("Can't listen!");
        //std::cerr << "Can't listen!";
        //return SRVERROR;
    }
    //return SRVNORM;
}

// Select clients
void server::Server::select_cts() {
    FD_ZERO(&bits_fd);
    FD_SET(listener, &bits_fd);

    for(auto i : clients_sockets)
        FD_SET(i, &bits_fd);

    if(select(max_fd, &bits_fd, (fd_set*)0, (fd_set*)0, &timeout) == -1) {
        throw std::runtime_error("Select call failed");
        //std::cerr << "Select call failed" << std::endl;
        //return SRVERROR;
    }
    //return SRVNORM;
}

// Accept new client
void server::Server::accept_srv() {
    if(FD_ISSET(listener, &bits_fd)){
        sockaddr_in client;
        socklen_t client_size = sizeof(client);
        int ct_socket = accept(listener, (sockaddr*)&client, &client_size);
        clients_sockets.push_back(ct_socket);
        max_fd = std::max(max_fd, ct_socket+1);

        if(clients_sockets.back() == -1) {
            throw std::runtime_error("Problem with client connecting");
            //std::cerr << "Problem with client connecting!";
            //return SRVERROR;
        }
        else {

#ifdef CTNAME
            memset(host, 0, NI_MAXHOST);
            memset(svc, 0, NI_MAXSERV);

            if(getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, svc, NI_MAXSERV, 0)) {
                std::cout << host << " connected on " << svc << std::endl;
            }
            else {
                inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
                std::cout << host << " connected on " << ntohs(client.sin_port) << std::endl;
            }
#endif

        }
        //return SRVNORM;
    }
    //return SRVNOCONN;
}

// Check clients' messages
void server::Server::check_cts() {
    for(cts_it = clients_sockets.begin(); cts_it != clients_sockets.end(); ) {
        if(FD_ISSET(*cts_it, &bits_fd)) {
            int bytes_rcv = read_srv(msg_buf);
            if(bytes_rcv == SRVERROR || bytes_rcv == SRVNORM) {
                close_ct();
                continue;
            }
            if(write_srv(msg_buf, bytes_rcv+1) == SRVERROR) {
                close_ct();
                continue;
            }
        }
        ++cts_it;
    }
}

// Write new message to client with ID: client_id
int server::Server::write_srv(const void* buffer, size_t msg_size) {
    if(send(*cts_it, buffer, msg_size, 0) == -1) return SRVERROR;
    else return SRVNORM;
}

// Read message from client with ID: client_id
int server::Server::read_srv(char* buffer) {
    memset(buffer, 0, sizeof(buffer));

    //int bytes_rcv = recv(*cts_it, buffer, sizeof(buffer), 0);
    msg::Message msg;
    int bytes_rcv = msg::readMessage(*cts_it, msg);

    if(bytes_rcv == -1) {
        std::cerr << "There was a connection issue" << std::endl;
        return SRVERROR;
    }
    else{
        std::cout << "Received: " << static_cast<int>(msg.type) << std::endl;
    }

    // if(bytes_rcv == -1) {
    //     std::cerr << "There was a connection issue" << std::endl;
    //     return SRVERROR;
    // }
    // if(bytes_rcv == 0) {
    //     std::cout << "The client disconnected" << std::endl;
    //     return SRVNORM;
    // }

    // Display message
    // std::cout << "Received: " << std::string(buffer, 0, bytes_rcv) << std::endl;
    return msg.buf_length + 8;
}

// Close connection with client whose ID: client_id
void server::Server::close_ct() {
    if(close(*cts_it) == -1) {
        throw std::runtime_error("Couldn't close client's socket");
        //return SRVERROR;
    }
    else cts_it = clients_sockets.erase(cts_it);
    //return SRVNORM;
}

server::Server::~Server() {
    std::cout << "Disconnected" << std::endl;
}