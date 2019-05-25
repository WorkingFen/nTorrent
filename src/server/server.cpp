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
    while(clients.size() != 0) {
        cts_it = clients.begin();
        msg::Message reply(211);
        reply.sendMessage(cts_it->socket);
        close_ct();
    }
    close(listener);
}

server::file* server::Server::add_file(std::string name) {
    auto result = files.find(name);
    if(result != files.end()) return &(result->second);
    else return nullptr;
}

server::file* server::Server::add_file(int size, std::string name) {
    auto result = files.find(name);
    if(result != files.end()) return &(result->second);

    file tmp(name, size);
    auto insertion = files.insert({tmp.name, tmp});
    return &(insertion.first->second);
}

server::block* server::Server::add_block(server::file& foo, std::string hash) {
    for(auto i : foo.blocks) if(i.hash == hash) return &i;

    block tmp(hash);
    foo.blocks.push_back(tmp);
    return &(foo.blocks.back());
}

bool server::Server::add_block(server::file& foo, std::string hash, uint no) {
    if(no >= foo.blocks.size()) return false;

    for(uint i = 0; i < foo.blocks.size(); i++)
        if(foo.blocks[i].hash == hash && i == no) {
            add_owner(foo.blocks[i], &*cts_it);
            return true;
        }

    return false;
}

void server::Server::add_owner(server::block& foo, server::client* owner) {
    for(auto i : foo.owners) if(i->socket == owner->socket) return;

    foo.owners.push_back(owner);
}

server::block* server::Server::get_block(server::file& foo, uint no) {
    if(no < foo.blocks.size()) return &(foo.blocks[no]);
    else return nullptr;
}

void server::Server::delete_file(std::map<std::string, server::file>::iterator it) {
    files.erase(it);
}

bool server::Server::delete_block(server::file& foo, int no) {
    for(auto i = foo.blocks.begin(); i != foo.blocks.end(); i++)
        if(i->hash == foo.blocks[no].hash) {
            foo.blocks.erase(i);
            break;
        }
    
    if(foo.blocks.empty()) return true;
    else return false;
}

bool server::Server::delete_owner(server::block& foo) {
    for(auto i = foo.owners.begin(); i != foo.owners.end(); i++)
        if(&(*i)->socket == &cts_it->socket) {
            foo.owners.erase(i);
            break;
        }
    
    if(foo.owners.empty()) return true;
    else return false;
}

bool server::Server::delete_owner(server::block& foo, std::string addr) {
    std::string str_ip = addr.substr(0, 15);
    in_addr_t ip = stoi(str_ip);
    std::size_t pos = addr.find(":");
    std::string str_port = addr.substr(pos+1);
    in_port_t port = stoi(str_port);

    for(auto i = foo.owners.begin(); i != foo.owners.end(); i++)
        if(*(&(*i)->address.sin_port) == port && *(&(*i)->address.sin_addr.s_addr) == ip) {
            foo.owners.erase(i);
            break;
        }
    
    if(foo.owners.empty()) return true;
    else return false;
}

std::pair<server::client*, int> server::Server::find_least_occupied(server::file& foo, std::vector<int> no_blocks) {
    client* best_ct = foo.blocks[0].owners.front();
    int best_no = 0;

    for(uint i = 0; i < foo.blocks.size(); i++) {
        if(!no_blocks.empty()) for(uint tmp : no_blocks) if(tmp == i) continue;

        for(auto j : foo.blocks[i].owners) {
            if(j->no_leeches == 0) return std::pair<client*, int>({j,i});
            else if(best_ct->no_leeches >= j->no_leeches) {
                best_ct = j;
                best_no = i;
            }
        }
    }

    return std::pair<client*, int>({best_ct,best_no});
}

server::Server::Server(const char srv_ip[15], const int& srv_port) : sigint_flag(false) {
    set_SIGmask();
    signal_thread = std::thread(&Server::signal_waiter, this);

    timeout.tv_sec = 120;
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

    for(auto i : clients)
        FD_SET(i.socket, &bits_fd);

    if(select(max_fd, &bits_fd, (fd_set*)0, (fd_set*)0, &timeout) == -1)
        throw std::runtime_error("Select call failed");
}

// Accept new client
void server::Server::accept_srv() {
    if(FD_ISSET(listener, &bits_fd)){
        sockaddr_in ct_addr;
        socklen_t client_size = sizeof(ct_addr);
        int ct_socket = accept(listener, (sockaddr*)&ct_addr, &client_size);
        client ct(ct_addr, ct_socket);
        clients.push_back(ct);
        max_fd = std::max(max_fd, ct_socket+1);

        if(clients.back().socket == -1)
            throw std::runtime_error("Problem with client connecting");
        else {
            msg::Message reply(210);
            reply.writeInt(pieceSize);
            reply.sendMessage(ct_socket);
#ifdef CTNAME
            memset(host, 0, NI_MAXHOST);
            memset(svc, 0, NI_MAXSERV);

            if(getnameinfo((sockaddr*)&ct_addr, sizeof(ct_addr), host, NI_MAXHOST, svc, NI_MAXSERV, 0)) {
                std::cout << host << " connected on " << svc << std::endl;
            }
            else {
                inet_ntop(AF_INET, &ct_addr.sin_addr, host, NI_MAXHOST);
                std::cout << host << " connected on " << ntohs(ct_addr.sin_port) << std::endl;
            }
#endif
        }
    }
}

// Check clients' messages
void server::Server::check_cts() {
    for(cts_it = clients.begin(); cts_it != clients.end(); ) {
        if(FD_ISSET(cts_it->socket, &bits_fd)) {
            int bytes_rcv = read_srv();
            if(bytes_rcv == SRVERROR || bytes_rcv == SRVNOCONN) {
                close_ct();
                continue;
            }
        }
        ++cts_it;
    }
}

// Read message from client with ID: client_id
int server::Server::read_srv() {
    if(!msg_manager.assembleMsg(cts_it->socket)) {
        if(msg_manager.lastReadResult()<=0) return SRVERROR;
        else return SRVNORM;
    }

    msg::Message msg = msg_manager.readMsg(cts_it->socket);

    if(msg.type == -1) {
#ifdef LOGS
        std::cerr << "There was a connection issue" << std::endl;
#endif        
        return SRVERROR;
    }
    else if(msg.type == 101) {
        file* foo = add_file(msg.readInt(), msg.readString(msg.readInt()));  // Weird specification of C++ compiler?
        for(int i = 0; msg.buf_length > 0; i++) {
            block* bar = add_block(*foo, msg.readString(64));
            add_owner(*bar, &*cts_it);
        }
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
        std::cout << "File name: " << foo->name << std::endl;
        std::cout << "File size: " << foo->size << std::endl;
        for(auto i : foo->blocks)
            std::cout << "Piece hash: " << i.hash << std::endl;
#endif
#ifdef EXPERT
        for(auto i : files) {
            std::cout << std::endl << "File: " << i.second.name << std::endl;
            for(int j = 0; j < i.second.blocks.size(); j++) {
                std::cout << j << ". Block: " << i.second.blocks[j].hash << std::endl;
                for(auto k : i.second.blocks[j].owners) {
                    std::cout << "--- " << k->address.sin_addr.s_addr << ":" << k->address.sin_port << std::endl;
                }
            }
        }
#endif
    }
    else if(msg.type == 102) { 
        for(auto i : files) {
            msg::Message file_list(201);
            file_list.writeInt(i.first.size());
            file_list.writeString(i.first);
            file_list.writeInt(i.second.size);
            file_list.sendMessage(cts_it->socket);
        }
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
#endif
    }
    else if(msg.type == 103) {
        auto foo = files.find(msg.readString(msg.readInt()));
        int no_block = msg.readInt();
        if(delete_owner(*get_block(foo->second, no_block)) && delete_block(foo->second, no_block)){
            std::string name = foo->second.name;
            delete_file(foo);
#ifdef LOGS
            std::cout << std::endl << "File " << name << " has been deleted" << std::endl;
#endif
        }
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
#endif
    }
    else if(msg.type == 104) {
        auto foo = files.find(msg.readString(msg.readInt()));
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
#endif
        msg::Message file_cfg(201);
        file_cfg.writeInt(foo->second.name.size());
        file_cfg.writeString(foo->second.name);
        file_cfg.writeInt(foo->second.size);
        file_cfg.sendMessage(cts_it->socket);
    }
    else if(msg.type == 105) {
        file* foo = add_file(msg.readString(msg.readInt()));
        int no_block = msg.readInt();
        if(!(foo != nullptr && add_block(*foo, msg.readString(64), no_block))) {}  // Wywal blad albo cos
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
        std::cout << "File name: " << foo->name << std::endl;
        std::cout << "Piece number: " << no_block << std::endl;
        std::cout << "Piece hash: " << foo->blocks[no_block].hash << std::endl;
#endif
    }
    else if(msg.type == 106) {
        auto foo = files.find(msg.readString(msg.readInt()));
        std::vector<int> blocks_no = std::vector<int>();
        while(msg.buf_length > 0)
            blocks_no.push_back(msg.readInt());
        auto bar = find_least_occupied(foo->second, blocks_no);
        bar.first->no_leeches++;
        msg::Message file_info(202);
        file_info.writeInt(foo->second.name.size());
        file_info.writeString(foo->second.name);
        file_info.writeInt(bar.second);
        file_info.writeString(foo->second.blocks[bar.second].hash);
        std::string addr = std::to_string(bar.first->address.sin_addr.s_addr) + ":" + std::to_string(bar.first->address.sin_port); 
        file_info.writeInt(addr.size());
        file_info.writeString(addr);
        file_info.sendMessage(cts_it->socket);
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
#endif
    }
    else if(msg.type == 107) {
        auto foo = files.find(msg.readString(msg.readInt()));
        int no_block = msg.readInt();
        std::string addr = msg.readString(msg.readInt());
        std::cout << std::endl << addr << std::endl;
        if(delete_owner(*get_block(foo->second, no_block), addr) && delete_block(foo->second, no_block)){
            std::string name = foo->second.name;
            delete_file(foo);
#ifdef LOGS
            std::cout << std::endl << "File " << name << " has been deleted" << std::endl;
#endif
        }
#ifdef LOGS
        std::cout << std::endl << "Message type: " << msg.type << std::endl;
#endif
    }
    else if(msg.type == 110) {
#ifdef KEEPALIVE
        std::cout << "KeepAlive: on socket number " << cts_it->socket << std::endl;
#endif
        msg::Message reply(209);
        reply.sendMessage(cts_it->socket);
    }
    else if(msg.type == 111) {
#ifdef LOGS
        std::cout << "The client disconnected" << std::endl;
#endif
        return SRVNOCONN;
    }
    else {
#ifdef LOGS
        std::cout << "Received: " << msg.type << std::endl;
        std::cout << "Message type is unknown" << std::endl;
        std::cout << "Message: ";
        for(char c : msg.buffer) std::cout << c;
        std::cout << std::endl;
        std::cout << "Message bytes: " << std::endl;
        for(char c : msg.buffer) std::cout << static_cast<int>(c) << " ";
        std::cout << std::endl;
#endif
    }

    return SRVNORM;
}

// Close connection with client whose ID: client_id
void server::Server::close_ct() {
#ifdef LOGS
    memset(host, 0, NI_MAXHOST);
    memset(svc, 0, NI_MAXSERV);

    if(getnameinfo((sockaddr*)&(cts_it->address), sizeof(cts_it->address), host, NI_MAXHOST, svc, NI_MAXSERV, 0)) {
        std::cout << "Closing socket of: " << host << ":" << svc << std::endl;
    }
    else {
        inet_ntop(AF_INET, &(cts_it->address).sin_addr, host, NI_MAXHOST);
        std::cout << "Closing socket of: " << host << ":" << ntohs(cts_it->address.sin_port) << std::endl;
    }
#endif
    if(close(cts_it->socket) == -1)
        throw std::runtime_error("Couldn't close client's socket");
    else cts_it = clients.erase(cts_it);
}

server::Server::~Server() {
#ifdef LOGS
    std::cout << "Disconnected" << std::endl;
#endif
}