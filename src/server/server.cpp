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

server::file& server::Server::add_file(std::string name, int size) {
    for(auto i : files) if(i.name == name) return i;

    file tmp;
    tmp.name = name;
    files.push_back(tmp);
    return tmp;
}

server::block& server::Server::add_block(server::file& foo, int no, std::string hash) {
    for(auto i : foo.blocks) if(i.hash == hash) return i;

    block tmp;
    tmp.no = no;
    tmp.hash = hash;
    foo.blocks.push_back(tmp);
    return tmp;
}

void server::Server::add_owner(server::block& foo, int owner) {
    for(auto i : foo.owners) if(i == owner) return;

    foo.owners.push_back(owner);
}

server::Server::Server(const char srv_ip[15], const int& srv_port) : sigint_flag(false) {
    set_SIGmask();
    signal_thread = std::thread(&Server::signal_waiter, this);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    socket_srv();
    int enable = 1;
    if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        std::cerr << "setting SO_REUSEADDR option on socket " << listener << " failed" << std::endl;

    bind_srv(srv_ip, srv_port);
    listen_srv();
    while(!sigint_flag){
        select_cts();
        accept_srv();
        check_cts();
    }
    pthread_cancel(signal_thread.native_handle());
    signal_thread.join();
    shutdown();
}

// Create listener socket
void server::Server::socket_srv() {
    if((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        throw std::runtime_error("Can't create a socket!");

    max_fd = listener + 1;
}

// Bind socket to ip:port 
void server::Server::bind_srv(const char srv_ip[15], const int& srv_port) {
    server.sin_family = AF_INET;
    server.sin_port = htons(srv_port);
    if(inet_pton(AF_INET, srv_ip, &server.sin_addr) == -1)
        throw std::domain_error("Improper IPv4 address");

    if(bind(listener, (sockaddr*)&server, sizeof(server)) == -1)
        throw std::runtime_error("Can't bind to IP:port");

#ifdef SOCKNAME
    socklen_t sa_len = sizeof(server);
    if(getsockname(listener, (sockaddr*)&server, &sa_len) == -1)
        throw std::runtime_error("Can't get socket name");    
    std::cout << "Successfully connected and listening at: " << srv_ip << ":" << ntohs(server.sin_port) << std::endl;
#endif
}

// Start listening on listener socket, with SOMAXCONN connections
void server::Server::listen_srv() {
    if(listen(listener, SOMAXCONN) == -1)
        throw std::runtime_error("Can't listen!");
}

// Select clients
void server::Server::select_cts() {
    FD_ZERO(&bits_fd);
    FD_SET(listener, &bits_fd);

    for(auto i : clients_sockets)
        FD_SET(i, &bits_fd);

    if(select(max_fd, &bits_fd, (fd_set*)0, (fd_set*)0, &timeout) == -1)
        throw std::runtime_error("Select call failed");
}

// Accept new client
void server::Server::accept_srv() {
    if(FD_ISSET(listener, &bits_fd)){
        sockaddr_in client;
        socklen_t client_size = sizeof(client);
        int ct_socket = accept(listener, (sockaddr*)&client, &client_size);
        clients_sockets.push_back(ct_socket);
        max_fd = std::max(max_fd, ct_socket+1);

        if(clients_sockets.back() == -1)
            throw std::runtime_error("Problem with client connecting");
        else {

        msg::Message hello(210);
        hello.writeInt(pieceSize);
        hello.sendMessage(ct_socket);

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
    }
}

// Check clients' messages
void server::Server::check_cts() {
    for(cts_it = clients_sockets.begin(); cts_it != clients_sockets.end(); ) {
        if(FD_ISSET(*cts_it, &bits_fd)) {
            int bytes_rcv = read_srv(msg_buf);
            if(bytes_rcv == SRVERROR || bytes_rcv == SRVNOCONN) {
                close_ct();
                continue;
            }
        }
        ++cts_it;
    }
}

// Write new message to client with ID: client_id
int server::Server::write_srv(const void* buffer, size_t msg_size) {
    // TODO: Depending on Message
    if(send(*cts_it, buffer, msg_size, 0) == -1) return SRVERROR;
    else return SRVNORM;
}

// Read message from client with ID: client_id
int server::Server::read_srv(char* buffer) {
    if(!msg_manager.assembleMsg(*cts_it))
        return msg_manager.lastReadResult();

    msg::Message msg = msg_manager.readMsg(*cts_it);

    if(msg.type == -1) {
#ifdef LOGS
        std::cerr << "There was a connection issue" << std::endl;
#endif        
        return SRVERROR;
    }
    else if(msg.type == 101) {
        int name_size = msg.readInt();
        /*file foo = add_file(msg.readString(name_size), msg.readInt());
        block bar;
        for(int i = 0; msg.buf_length > 0; i++) {
            bar = add_block(foo, i, msg.readString(64));
            add_owner(bar, *cts_it);
        }*/
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
        std::cout << "File name: " << msg.readString(name_size) << std::endl;
        std::cout << "File size: " << msg.readInt() << std::endl;
        while(msg.buf_length >= 64)
            std::cout << "Piece hash: " << msg.readString(64) << std::endl;
#endif
    }/*
    else if(msg.type == 102) {
        msg::Message file_list(203);   // TODO

        for(auto i : files) {
            file_list.writeInt(i.name.size());
            file_list.writeString(i.name);
            file_list.writeInt(i.size);
        }
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
#endif
    }
    else if(msg.type == 103) {
// TODO:
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
#endif
        int name_size = msg.readInt();     

        std::cout << "File name: " << msg.readString(name_size) << std::endl;
        std::cout << "Piece hash: " << msg.readString(64) << std::endl;
    }
    else if(msg.type == 104) {
// TODO:
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
#endif
        int name_size = msg.readInt();     

        std::cout << "File name: " << msg.readString(name_size) << std::endl;
    }
    else if(msg.type == 105) {
        int name_size = msg.readInt();
        file foo = add_file(msg.readString(name_size), 0);
        block bar = add_block(foo, msg.readInt(), msg.readString(64));
        add_owner(bar, *cts_it);     
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
        std::cout << "File name: " << foo.name << std::endl;
        std::cout << "Piece number: " << bar.no << std::endl;
        std::cout << "Piece hash: " << bar.hash << std::endl;
#endif
    }*/
    else if(msg.type == 106) {
// TODO: ????
        int name_size = msg.readInt();
        // file foo = get_file()                // TODO

        std::cout << "File name: " << msg.readString(name_size) << std::endl;
        std::cout << "File size: " << msg.readInt() << std::endl;
        while(msg.buf_length > 0)
            std::cout << "Piece hash: " << msg.readString(64) << std::endl;    
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
#endif
    }
    else if(msg.type == 107) {
// TODO: ????
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
#endif
        int name_size = msg.readInt();     

        std::cout << "File name: " << msg.readString(name_size) << std::endl;
        std::cout << "File size: " << msg.readInt() << std::endl;
        while(msg.buf_length > 0)
            std::cout << "Piece hash: " << msg.readString(64) << std::endl;
    }
    else if(msg.type == 110) {
#ifdef LOGS
        std::cout << std::endl << "KeepAlive: on socket number " << *cts_it << std::endl;
#endif
    }
    else if(msg.type == 111) {
#ifdef LOGS
        std::cout << "The client disconnected" << std::endl;
#endif
        return SRVNOCONN;
    }
    else {
        std::cout << "Received: " << msg.type << std::endl;
        std::cout << "Message: ";
        for(char c : msg.buffer) std::cout << c;
        std::cout << std::endl;
        std::cout << "Message bytes: " << std::endl;
        for(char c : msg.buffer) std::cout << static_cast<int>(c) << " ";
        std::cout << std::endl;
    }

    return SRVNORM;
}

// Close connection with client whose ID: client_id
void server::Server::close_ct() {
    if(close(*cts_it) == -1)
        throw std::runtime_error("Couldn't close client's socket");
    else cts_it = clients_sockets.erase(cts_it);
}

server::Server::~Server() {
#ifdef LOGS
    std::cout << "Disconnected" << std::endl;
#endif
}