#include "headers/server.hpp"

void server::Server::show_ct_ofiles() {
    std::cout << std::endl << GOLDF;
    std::cout << "Owned files: " << std::endl;
    std::cout << ORANGEF << "###" << std::endl;
    for(auto o_file : cts_it->o_files) {
        std::cout << "# " << o_file.first << " - [";
        int c_num;
        bool cont = false;
        bool first = true;
        for(auto e_block_num : o_file.second) {
            if(first) {
                c_num = e_block_num.first + 1;
                first = false;
                std::cout << e_block_num.first;
            }
            else if(e_block_num.first == c_num) {
                cont = true;
                c_num++;
            }
            else {
                if(cont) std::cout << "-" << c_num-1 << ", " << e_block_num.first;
                else std::cout << ", " << e_block_num.first;
                c_num = e_block_num.first + 1;
            }
        }
        if(cont) std::cout << "-" << c_num-1 << "]";
        else std::cout << "]";
        std::cout << std::endl;
    }
    std::cout << GOLDF;
}

void server::Server::show_ct_dfiles() {
    std::cout << "Downloading files: " << std::endl;
    std::cout << ORANGEF << "###" << std::endl;
    for(auto d_file : cts_it->d_files) {
        std::cout << "# " << d_file.first << " - [";
        int c_num;
        bool cont = false;
        bool first = true;
        for(auto e_block_num : d_file.second) {
            if(first) {
                c_num = e_block_num.first + 1;
                first = false;
                std::cout << e_block_num.first;
            }
            else if(e_block_num.first == c_num) {
                cont = true;
                c_num++;
            }
            else {
                if(cont) std::cout << "-" << c_num-1 << ", " << e_block_num.first;
                else std::cout << ", " << e_block_num.first;
                c_num = e_block_num.first + 1;
            }
        }
        if(cont) std::cout << "-" << c_num-1 << "]";
        else std::cout << "]";
        std::cout << std::endl;
    }
    std::cout << RESETF;
}

void server::Server::show_srvfiles(server::file& c_file) {
    std::map<client*, std::vector<int>> client_info;
    std::cout << std::endl << ORANGEF;
    std::cout << "File: " << c_file.name << std::endl;
    for(uint block_num = 0; block_num < c_file.blocks.size(); block_num++) {
    #ifdef FILESADV
        std::cout << block_num << ". Block: " << c_file.blocks[block_num].hash << std::endl;
    #endif
        for(auto c_owner : c_file.blocks[block_num].owners) {
            auto c_client = client_info.find(c_owner);
            if(c_client == client_info.end()) {
                std::vector<int> no_blocks;
                no_blocks.push_back(block_num);
                client_info.insert({c_owner, no_blocks});
            }
            else c_client->second.push_back(block_num);
        }
    }
    for(auto e_client : client_info) {
        std::cout << "--- ";
        std::cout << MINTF << e_client.first->call_addr.sin_addr.s_addr;
        std::cout << ":" << e_client.first->call_addr.sin_port << std::endl;
        std::cout << BGREENF << "# ";
        int c_num;
        bool cont = false;
        for(auto e_block_num : e_client.second) {
            if(e_block_num == e_client.second.front()) {
                c_num = e_block_num;
                std::cout << "[" << e_block_num;
            }
            if(e_block_num != c_num + 1 && e_block_num != c_num) {
                if(cont) std::cout << "-" << c_num << ", " << e_block_num;
                else std::cout << ", " << e_block_num;
                cont = false;
            }
            else cont = true;
            c_num = e_block_num;
            if(e_block_num == e_client.second.back()) {
                if(cont) std::cout << "-" << c_num << "]";
                else std::cout << "]";
            }
        }
        std::cout << std::endl;
    }
    std::cout << RESETF;
}

// Wait for SIGINT to occur
void server::Server::signal_waiter() {
    int sig;

    sigwait(&signal_set, &sig);
    if(sig == SIGINT) {
    #ifdef LOGS
        std::cout << REDF << BOLDF << "\rReceived SIGINT. Exiting...";
        std::cout << std::endl << RESETF;
    #endif
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

// Close all connections; Before that, send msg about closing
void server::Server::shutdown() {
    while(clients.size() != 0) {
        cts_it = clients.begin();
        msg::Message farewell(211);
        farewell.sendMessage(cts_it->socket);
        close_ct();
    }
    close(listener);
}

// Add new file and get it's pointer or get existing pointer
server::file* server::Server::add_file(int size, std::string name) {
    auto e_file = files.find(name);
    if(e_file != files.end()) return &(e_file->second);

    file n_file(name, size);
    auto i_file = files.insert({n_file.name, n_file});
    return &(i_file.first->second);
}

// Add new block and get it's pointer or get existing pointer
server::block* server::Server::add_block(server::file& c_file, std::string hash, uint no) {
    if(no < c_file.blocks.size()) { 
        if(c_file.blocks[no].hash == hash) return &c_file.blocks[no];
        else return nullptr;
    }
    else {
        block n_block(hash);
        c_file.blocks.push_back(n_block);
        return &(c_file.blocks.back());
    }
}

// Add new owner if block information is correct 
int server::Server::add_block_owner(server::file& c_file, std::string hash, uint no) {
    if(no >= c_file.blocks.size()) return MSG_RNGERROR;

    if(c_file.blocks[no].empty) c_file.blocks[no] = block(hash);

    if(c_file.blocks[no].hash == hash) {
        add_owner(c_file.blocks[no], &*cts_it);
        return MSG_OK;
    }
    else return MSG_ERROR;
}

// Add new owner to block information if not already there
void server::Server::add_owner(server::block& c_block, server::client* owner) {
    for(auto e_owner : c_block.owners) if(e_owner->socket == owner->socket) return;

    c_block.owners.push_back(owner);
}

// Get existing file by its name, else nullptr
server::file* server::Server::get_file(std::string name) {
    auto e_file = files.find(name);
    if(e_file != files.end()) return &(e_file->second);
    else return nullptr;
}

// Get existing block by its number, else nullptr
server::block* server::Server::get_block(server::file& c_file, uint no) {
    if(no < c_file.blocks.size()) return &(c_file.blocks[no]);
    else return nullptr;
}

// Delete file from vector of files
void server::Server::delete_file(server::file& c_file) {
    auto it = files.find(c_file.name);
    files.erase(it);
}

// Delete block by overwriting it
bool server::Server::delete_block(server::file& c_file, int no) {
    c_file.blocks[no] = block();

    for(auto c_block : c_file.blocks) if(!c_block.empty) return false;
    return true;
}

// Delete current client from owners list if block isn't empty and it's in there, then check if
// block's owners vector isn't empty
bool server::Server::delete_owner(server::block& c_block) {
    if(c_block.empty) return false;

    for(auto c_owner = c_block.owners.begin(); c_owner != c_block.owners.end(); c_owner++)
        if(&(*c_owner)->socket == &cts_it->socket) {
            c_block.owners.erase(c_owner);
            break;
        }
    
    if(c_block.owners.empty()) return true;
    else return false;
}

// Delete owner if it exists and if block isn't empty, then check if block's owners vector isn't empty
bool server::Server::delete_owner(server::block& c_block, uint ip, int port) {
    if(c_block.empty) return false;

    for(auto c_owner = c_block.owners.begin(); c_owner != c_block.owners.end(); c_owner++)
        if(*(&(*c_owner)->call_addr.sin_port) == port && *(&(*c_owner)->call_addr.sin_addr.s_addr) == ip) {
            c_block.owners.erase(c_owner);
            break;
        }
    
    if(c_block.owners.empty()) return true;
    else return false;
}

// Get least occupied seeder and its block which client doesn't have
std::pair<server::client*, int> server::Server::find_least_occupied(server::file& c_file, std::vector<int> no_blocks) {
    client* best_ct;
    int best_no;
    bool got;

    for(uint i = 0; i <= c_file.blocks.size(); i++) {
        if(i == c_file.blocks.size()) return std::pair<client*, int>({nullptr,i});
        if(!c_file.blocks[i].empty) {
            got = false;
            if(!no_blocks.empty()) {
                for(uint tmp : no_blocks)
                    if(tmp == i) {
                        got = true;
                        break;
                    }
            }
            if(got) continue;
            best_ct = c_file.blocks[i].owners.front();
            best_no = i;
            if(best_ct->no_leeches == 0) return std::pair<client*, int>({best_ct, best_no});
            break;
        }
    }
    
    for(uint i = best_no + 1; i < c_file.blocks.size(); i++) {
        if(c_file.blocks[i].empty) continue;

        got = false;
        if(!no_blocks.empty()) {
            for(uint tmp : no_blocks)
                if(tmp == i) {
                    got = true;
                    break;
                }
        }
        if(got) continue;
        
        for(auto j : c_file.blocks[i].owners) {
            if(j->no_leeches == 0) return std::pair<client*, int>({j,i});
            else if(best_ct->no_leeches > j->no_leeches) {
                best_ct = j;
                best_no = i;
            }
        }
    }

    return std::pair<client*, int>({best_ct,best_no});
}

// Check if client is still alive
bool server::Server::still_alive() {
    t_point now = std::chrono::high_resolution_clock::now();
    auto rslt = std::chrono::duration_cast<std::chrono::seconds>(now - cts_it->timeout);
    if(rslt >= timesup) return false;
    else return true;
}

// Set leeches of client's downloaded file
void server::Server::set_leeches(std::pair<server::client*, int>& lo_ct, std::string name) {
    auto e_file = lo_ct.first->o_files.find(name);
    if(e_file != lo_ct.first->o_files.end()) {
        auto e_block = e_file->second.find(lo_ct.second);
        if(e_block != e_file->second.end()) e_block->second++;
    }
}

// Set leeches of client's downloaded file with ip:port
void server::Server::set_leeches(std::string name, int no_block, uint ip, int port) {
    file* e_file = get_file(name);
    client* e_owner;
    if(e_file == nullptr) return;
    else {
        block* e_block = get_block(*e_file, no_block);
        if(e_block == nullptr) return;
        else {
            for(auto c_owner = e_block->owners.begin(); c_owner != e_block->owners.end(); c_owner++)
                if(*(&(*c_owner)->call_addr.sin_port) == port && *(&(*c_owner)->call_addr.sin_addr.s_addr) == ip) {
                    e_owner = *c_owner;
                    break;
                }
        }
    }

    auto o_file = e_owner->o_files.find(name);
    if(o_file != e_owner->o_files.end()) {
        auto o_block = o_file->second.find(no_block);
        if(o_block != o_file->second.end()) o_block->second--;
    }
}

// Set owned file to client
void server::Server::be_owned(std::string name, int no_block) {
    auto e_file = cts_it->o_files.find(name);
    
    if(e_file == cts_it->o_files.end()) {
        std::map<int,int> no_blocks;
        no_blocks.insert({no_block,0});
        cts_it->o_files.insert({name, no_blocks});
    }
    else {
        auto e_block = e_file->second.find(no_block);
        if(e_block != e_file->second.end()) return;
        else e_file->second.insert({no_block,0});
    }
}

// Set block (and file) to be not owned by client
void server::Server::not_owned(std::string name, int no_block) {
    auto e_file = cts_it->o_files.find(name);

    if(e_file != cts_it->o_files.end()) {
        auto e_block = e_file->second.find(no_block);
        if(e_block != e_file->second.end()) {
            cts_it->no_leeches -= e_block->second;
            e_file->second.erase(e_block);
            if(e_file->second.empty()) cts_it->o_files.erase(e_file);
        }
    }
}

// Set block (and file) to be not owned by client with ip:port
void server::Server::not_owned(std::string name, int no_block, uint ip, int port) {
    file* e_file = get_file(name);
    if(e_file == nullptr) return;
    else {
        block* e_block = get_block(*e_file, no_block);
        if(e_block == nullptr) return;
        else {
            for(auto c_owner = e_block->owners.begin(); c_owner != e_block->owners.end(); c_owner++)
                if(*(&(*c_owner)->call_addr.sin_port) == port && *(&(*c_owner)->call_addr.sin_addr.s_addr) == ip) {
                    auto o_file = (*c_owner)->o_files.find(name);
                    if(o_file != (*c_owner)->o_files.end()) {
                        auto o_block = o_file->second.find(no_block);
                        if(o_block != o_file->second.end()) {
                            (*c_owner)->no_leeches -= o_block->second;
                            o_file->second.erase(o_block);
                            if(o_file->second.empty()) (*c_owner)->o_files.erase(o_file);
                        }
                    }
                    break;
                }
        }
    }
}

// Add new file (or just new block) to map of to be downloaded files
void server::Server::be_downloaded(std::string name, int no_block, uint ip, int port) {
    auto e_file = cts_it->d_files.find(name);
    
    sockaddr_in owner;
    owner.sin_addr.s_addr = ip;
    owner.sin_port = port;
    if(e_file == cts_it->d_files.end()) {    
        std::map<int,sockaddr_in> o_blocks;
        o_blocks.insert({no_block, owner});
        cts_it->d_files.insert({name, o_blocks});
    }
    else {
        auto e_block = e_file->second.find(no_block);
        if(e_block != e_file->second.end()) return;
        else e_file->second.insert({no_block, owner});
    }
}

// Delete file of client from downloaded files and change no_leeches in 
void server::Server::release_downloaded(std::string name, int no_block, uint ip, int port) {
    file* e_file = get_file(name);
    if(e_file == nullptr) return;
    else {
        block* e_block = get_block(*e_file, no_block);
        if(e_block == nullptr) return;
        else {
            for(auto c_owner = e_block->owners.begin(); c_owner != e_block->owners.end(); c_owner++)
                if(*(&(*c_owner)->call_addr.sin_port) == port && *(&(*c_owner)->call_addr.sin_addr.s_addr) == ip) {
                    auto o_file = (*c_owner)->o_files.find(name);
                    if(o_file != (*c_owner)->o_files.end()) {
                        auto o_block = o_file->second.find(no_block);
                        if(o_block != o_file->second.end()) {
                            o_block->second--;
                            (*c_owner)->no_leeches--;
                            auto d_file = cts_it->d_files.find(name);
                            if(d_file != cts_it->d_files.end()) {
                                auto d_block = d_file->second.find(no_block);
                                if(d_block != d_file->second.end()) {
                                    d_file->second.erase(d_block);
                                    if(d_file->second.empty()) cts_it->d_files.erase(d_file);
                                }
                            }
                        }
                    }
                    break;
                }
        }
    }
}

server::Server::Server(const char srv_ip[15], const int& srv_port) : timesup(8), sigint_flag(false) {
    set_SIGmask();
    signal_thread = std::thread(&Server::signal_waiter, this);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    socket_srv();
    bind_srv(srv_ip, srv_port);
    listen_srv();   
}

// Main program loop
void server::Server::run_srv() {
    int enable = 1;
    if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        std::cerr << "setting SO_REUSEADDR option on socket " << listener << " failed" << std::endl;

    while(!sigint_flag){
        select_cts();
        accept_srv();
        check_cts();
    }
    close_srv();
}

// Cancel server's thread and close its socket
void server::Server::close_srv() {
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

    #ifdef SRVNAME
    socklen_t sa_len = sizeof(server);
    if(getsockname(listener, (sockaddr*)&server, &sa_len) == -1)
        throw std::runtime_error("Can't get socket name");
    std::cout << SKYF << BOLDF << "Successfully connected and listening at: ";
    std::cout << MINTF << srv_ip << ":" << ntohs(server.sin_port);
    std::cout << std::endl << RESETF;
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
                std::cout << MINTF << host;
                std::cout << GOLDF << " connected on ";
                std::cout << MINTF << svc;
            }
            else {
                inet_ntop(AF_INET, &ct_addr.sin_addr, host, NI_MAXHOST);
                std::cout << MINTF << host;
                std::cout << GOLDF << " connected on ";
                std::cout << MINTF << ntohs(ct_addr.sin_port);
            }
            std::cout << std::endl << RESETF;
            #endif
        }
    }
}

// Check clients' messages
void server::Server::check_cts() {
    for(cts_it = clients.begin(); cts_it != clients.end(); ) {
        if(!still_alive()) {
            close_ct();
            continue;
        }
        if(FD_ISSET(cts_it->socket, &bits_fd)) {
            int bytes_rcv = read_srv();
            if(bytes_rcv == SRV_ERROR || bytes_rcv == SRV_NOCONN) {
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
        if(msg_manager.lastReadResult() <= 0) return SRV_ERROR;
        else return SRV_OK;
    }

    msg::Message msg = msg_manager.readMsg(cts_it->socket);

    #ifdef MSGINFO
    std::cout << BGREENF << std::endl << "Message type: " << msg.type << "\t";
    #ifdef MSGADV
    std::cout << msg.getType(msg.type);
    #endif
    std::cout << RESETF;
    #endif
    if(msg.type == -1) {
        #ifdef ERROR
        std::cerr << std::endl << REDF << BOLDF;
        std::cerr << "There was a connection issue";
        std::cerr << std::endl << RESETF;
        #endif        
        return SRV_ERROR;
    }
    // Share file
    else if(msg.type == 101) {
        file* c_file = add_file(msg.readInt(), msg.readString(msg.readInt()));  // Weird specification of C++ compiler?
        for(uint no_block = 0; msg.buf_length > 0; no_block++) {
            block* c_block = add_block(*c_file, msg.readString(64), no_block); 
            if(c_block == nullptr) {                        // Error: Hashes don't match
                msg::Message bad_hash(203);
                bad_hash.writeInt(c_file->name.size());
                bad_hash.writeString(c_file->name);
                bad_hash.writeInt(no_block);
                bad_hash.sendMessage(cts_it->socket);
            }             
            else {
                add_owner(*c_block, &*cts_it);
                be_owned(c_file->name, no_block);
            }
        }
        #ifdef MSGADV
        std::cout << std::endl << BGREENF;
        std::cout << "File name: " << c_file->name << std::endl;
        std::cout << "File size: " << c_file->size << std::endl;
        for(auto c_block : c_file->blocks)
            std::cout << "Piece hash: " << c_block.hash << std::endl;
        std::cout << RESETF;
        #endif
        #ifdef OWNEDFILES
        show_srvfiles(*c_file);
        #endif
    }
    // List all files
    else if(msg.type == 102) {
        if(!files.empty())
            for(auto c_file : files) {
                msg::Message file_list(201);
                file_list.writeInt(c_file.first.size());
                file_list.writeString(c_file.first);
                file_list.writeInt(c_file.second.size);
                file_list.sendMessage(cts_it->socket);
                #ifdef FILES
                std::cout << std::endl << BGREENF;
                std::cout << "File name: " << c_file.first << "\t";
                std::cout << "Size: " << c_file.second.size;
                std::cout << std::endl << RESETF;
                #ifdef OWNEDFILES
                show_srvfiles(c_file.second);
                show_ct_ofiles();
                #endif
                #endif
            }
        else {                                      // Error: There are no files
            msg::Message file_list(207);
            file_list.sendMessage(cts_it->socket);
        }            
    }
    // Delete client's file
    else if(msg.type == 103) {
        std::string name = msg.readString(msg.readInt());
        file* e_file = get_file(name);
        if(e_file == nullptr) {                     // Error: There is no file with this name
            msg::Message bad_name(204);
            bad_name.writeInt(name.size());
            bad_name.writeString(name);
            bad_name.sendMessage(cts_it->socket);
        }        
        else {
            block* e_block;
            for(uint no_block = 0; no_block < e_file->blocks.size(); no_block++) {
                e_block = &(e_file->blocks[no_block]);
                std::string name = e_file->name;
                if(delete_owner(*e_block) && delete_block(*e_file, no_block)){
                    delete_file(*e_file);   
                    #ifdef FILES
                    std::cout << std::endl << ORANGEF;
                    std::cout << "File " << name << " has been deleted";
                    std::cout << std::endl << RESETF;
                    #endif
                }
                not_owned(name, no_block);
            }
        }
        #ifdef MSGADV
        std::cout << std::endl << BGREENF;
        std::cout << "File name: " << e_file->name;
        std::cout << std::endl << RESETF;
        #endif
    }
    // File download request
    else if(msg.type == 104) {
        std::string name = msg.readString(msg.readInt());
        file* c_file = get_file(name);
        if(c_file == nullptr) {                     // Error: There is no file with this name
            msg::Message bad_name(204);
            bad_name.writeInt(name.size());
            bad_name.writeString(name);
            bad_name.sendMessage(cts_it->socket);
        }            
        else {
            msg::Message file_cfg(201);
            file_cfg.writeInt(c_file->name.size());
            file_cfg.writeString(c_file->name);
            file_cfg.writeInt(c_file->size);
            file_cfg.sendMessage(cts_it->socket);
        }
        #ifdef MSGADV
        std::cout << std::endl << BGREENF;
        std::cout << "File name: " << e_file->name;
        std::cout << std::endl << RESETF;
        #endif
    }
    // Inform about block possession
    else if(msg.type == 105) {
        #ifdef OWNEDFILES
        show_ct_dfiles();
        #endif
        std::string name = msg.readString(msg.readInt());
        file* c_file = get_file(name);
        if(c_file == nullptr) {                     // Error: There is no file with this name
            msg::Message bad_name(204);
            bad_name.writeInt(name.size());
            bad_name.writeString(name);
            bad_name.sendMessage(cts_it->socket);
        }            
        else {
            int no_block = msg.readInt();
            switch(add_block_owner(*c_file, msg.readString(64), no_block)) {
                case MSG_RNGERROR: {                // Error: There is no block in that position
                    msg::Message bad_block(205);
                    bad_block.writeInt(c_file->name.size());
                    bad_block.writeString(c_file->name);
                    bad_block.writeInt(no_block);
                    bad_block.sendMessage(cts_it->socket);         
                    break;
                }
                case MSG_ERROR: {                   // Error: Hashes don't match
                    msg::Message bad_hash(203);
                    bad_hash.writeInt(c_file->name.size());
                    bad_hash.writeString(c_file->name);
                    bad_hash.writeInt(no_block);
                    bad_hash.sendMessage(cts_it->socket);        
                    break;
                }
            }
            be_owned(c_file->name, no_block);
            auto d_file = cts_it->d_files.find(c_file->name);
            if(d_file != cts_it->d_files.end()) {
                auto d_block = d_file->second.find(no_block);
                if(d_block != d_file->second.end()) {
                    release_downloaded(c_file->name, no_block, d_block->second.sin_addr.s_addr, d_block->second.sin_port);
                }
            }
            #ifdef MSGADV
            std::cout << std::endl << GREENF;
            std::cout << "File name: " << c_file->name << std::endl;
            std::cout << "Piece number: " << no_block << std::endl;
            std::cout << "Piece hash: " << c_file->blocks[no_block].hash;
            std::cout << std::endl << RESETF;
            #endif
            #ifdef OWNEDFILES
            show_ct_ofiles();
            #endif
        }
    }
    // Give client information about block to download
    else if(msg.type == 106) {
        uint no_blocks;
        std::vector<int> blocks_no;
        std::string name = msg.readString(msg.readInt());
        file* c_file = get_file(name);
        if(c_file == nullptr) {                     // Error: There is no file with this name
            msg::Message bad_name(204);
            bad_name.writeInt(name.size());
            bad_name.writeString(name);
            bad_name.sendMessage(cts_it->socket);
        }
        else {
            no_blocks = msg.readInt();
            while(msg.buf_length > 0)
                blocks_no.push_back(msg.readInt());
    
            msg::Message file_info(202);
            std::vector<std::pair<server::client*, int>> lo_vector;
            for(uint i = 0; i < no_blocks; i++) {
                auto lo_ct = find_least_occupied(*c_file, blocks_no);
                if(lo_ct.first == nullptr) {        // Error: There are no blocks that could be downloaded
                    msg::Message no_block(206);
                    no_block.writeInt(c_file->name.size());
                    no_block.writeString(c_file->name);
                    no_block.sendMessage(cts_it->socket);
                    for(int block_num = c_file->blocks.size(); block_num > 0; block_num--) {
                        block c_block = c_file->blocks[block_num-1];
                        for(auto c_owner : c_block.owners) {
                            delete_owner(c_block, c_owner->call_addr.sin_addr.s_addr, c_owner->call_addr.sin_port);
                            not_owned(c_file->name, block_num-1, c_owner->call_addr.sin_addr.s_addr, c_owner->call_addr.sin_port);
                        }
                        delete_block(*c_file, block_num-1);
                    }
                    delete_file(*c_file);
                }
                else {
                    be_downloaded(c_file->name, lo_ct.second, lo_ct.first->call_addr.sin_addr.s_addr, lo_ct.first->call_addr.sin_port);
                    blocks_no.push_back(lo_ct.second);
                    lo_ct.first->no_leeches++;
                    set_leeches(lo_ct, c_file->name);
                    lo_vector.push_back(lo_ct);
                }
            }
            if(!lo_vector.empty()) {
                file_info.writeInt(c_file->name.size());
                file_info.writeString(c_file->name);
                file_info.writeInt(lo_vector.size());
                for(auto lo_ct : lo_vector) {
                    file_info.writeInt(lo_ct.second);
                    file_info.writeString(c_file->blocks[lo_ct.second].hash);
                    file_info.writeInt(lo_ct.first->call_addr.sin_addr.s_addr);
                    file_info.writeInt(lo_ct.first->call_addr.sin_port);
                }
                file_info.sendMessage(cts_it->socket);            
            }
        }
        #ifdef MSGADV
        std::cout << std::endl << GREENF;
        std::cout << "File name: " << c_file->name << std::endl;
        std::cout << "Number of blocks: " << no_blocks << std::endl;
        std::cout << std::endl << RESETF;
        #endif
    }
    // Wrong hash from seeder
    else if(msg.type == 107) {
        int no_block, o_addr, o_port;
        std::string name = msg.readString(msg.readInt());
        file* e_file = get_file(name);
        if(e_file == nullptr) {                     // Error: There is no file with this name
            msg::Message bad_name(204);
            bad_name.writeInt(name.size());
            bad_name.writeString(name);
            bad_name.sendMessage(cts_it->socket);
        }
        else {
            no_block = msg.readInt();
            o_addr = msg.readInt();
            o_port = msg.readInt();
            block* e_block = get_block(*e_file, no_block);
            if(e_block == nullptr) {               // Error: There is no block in that position
                msg::Message bad_block(205);
                bad_block.writeInt(e_file->name.size());
                bad_block.writeString(e_file->name);
                bad_block.writeInt(no_block);
                bad_block.sendMessage(cts_it->socket);
            }
            else {
                std::string name = e_file->name;
                not_owned(name, no_block, o_addr, o_port);
                if(delete_owner(*e_block, o_addr, o_port) && delete_block(*e_file, no_block)){
                    delete_file(*e_file);   
                    #ifdef LOGS
                    std::cout << std::endl << "File " << name << " has been deleted" << std::endl;
                    #endif
                }      
            }      
        }
        #ifdef MSGADV
        std::cout << std::endl << GREENF;
        std::cout << "File name: " << c_file->name << std::endl;
        std::cout << "Block number: " << no_block << std::endl;
        std::cout << "Address of client with bad hash: ";
        std::cout << MINTF << o_addr << ":" << o_port;
        std::cout << std::endl << RESETF;
        #endif
    }
    /*else if(msg.type == 108) {
        auto foo = files.find(msg.readString(msg.readInt()));
        int no_block = msg.readInt();
        if(delete_owner(*get_block(foo->second, no_block)) && delete_block(foo->second, no_block)){
            std::string name = foo->second.name;
            delete_file(foo);
            #ifdef LOGS
            std::cout << std::endl << "File " << name << " has been deleted" << std::endl;
            #endif
        }
    }*/
    // Client keepalive
    else if(msg.type == 110) {
        #ifdef KEEPALIVE
        std::cout << GOLDF << "[KeepAlive] Socket: ";
        std::cout << MINTF << cts_it->socket;
        std::cout << std::endl << RESETF;
        #endif
        cts_it->timeout = std::chrono::high_resolution_clock::now();
        msg::Message reply(209);
        reply.sendMessage(cts_it->socket);
    }
    // Dissconnect current client
    else if(msg.type == 111) {
        #ifdef LOGS
        std::cout << std::endl << GOLDF;
        std::cout << "Client disconnected";
        std::cout << std::endl << RESETF;
        #endif
        return SRV_NOCONN;
    }
    // Get address on which client is listening
    else if(msg.type == 112) {
        cts_it->call_addr.sin_addr.s_addr = msg.readInt();
        cts_it->call_addr.sin_port = msg.readInt();
        #ifdef MSGADV
        std::cout << std::endl <<BGREENF;
        std::cout << "Client's IP(uint): ";
        std::cout << MINTF << cts_it->call_addr.sin_addr.s_addr;
        std::cout << BGREENF << "\tPort: ";
        std::cout << MINTF << cts_it->call_addr.sin_port;
        std::cout << std::endl << RESETF;
        #endif
        #ifdef LOGS
        std::cout << std::endl << GOLDF;
        std::cout << "Client is listening on: ";
        std::cout << MINTF;
        std::cout << cts_it->call_addr.sin_addr.s_addr << ":" << cts_it->call_addr.sin_port;
        std::cout << std::endl << RESETF;
        #endif
    }
    else {
        #ifdef MSGINFO
        std::cout << std::endl << BGREENF;
        std::cout << "Message type is unknown" << std::endl;
        std::cout << "Message: ";
        for(char c : msg.buffer) std::cout << c;
        #ifdef MSGADV
        std::cout << std::endl << "Message bytes: " << std::endl;
        for(char c : msg.buffer) std::cout << static_cast<int>(c) << " ";
        #endif
        std::cout << std:: endl << RESETF;
        #endif
    }
    return SRV_OK;
}

// Close connection with client whose ID: client_id
void server::Server::close_ct() {
    #ifdef LOGS
    memset(host, 0, NI_MAXHOST);
    memset(svc, 0, NI_MAXSERV);

    if(getnameinfo((sockaddr*)&(cts_it->address), sizeof(cts_it->address), host, NI_MAXHOST, svc, NI_MAXSERV, 0)) {
        std::cout << GOLDF << "Closing socket of: ";
        std::cout << MINTF << host << ":" << svc << std::endl;
    }
    else {
        inet_ntop(AF_INET, &(cts_it->address).sin_addr, host, NI_MAXHOST);
        std::cout << GOLDF << "Closing socket of: ";
        std::cout << MINTF << host << ":" << ntohs(cts_it->address.sin_port) << std::endl;
    }
    std::cout << RESETF;
    #endif
    for(auto i : cts_it->o_files) {
        file* e_file = get_file(i.first);
        block* e_block;
        for(uint no_block = 0; no_block < e_file->blocks.size(); no_block++) {
            e_block = &(e_file->blocks[no_block]);
            std::string name = e_file->name;
            if(delete_owner(*e_block) && delete_block(*e_file, no_block)){
                delete_file(*e_file);   
                #ifdef FILES
                std::cout << std::endl << ORANGEF;
                std::cout << "File " << name << " has been deleted";
                std::cout << std::endl << RESETF;
                #endif
            }
            not_owned(name, no_block);
        }
    }
    
    for(auto i : cts_it->d_files)
        for(auto j : i.second) set_leeches(i.first, j.first, j.second.sin_addr.s_addr, j.second.sin_port);

    if(close(cts_it->socket) == -1)
        throw std::runtime_error("Couldn't close client's socket");
    else cts_it = clients.erase(cts_it);
}

server::Server::~Server() {
    #ifdef LOGS
    std::cout << BREDF << "Disconnected" << std::endl;
    std::cout << RESETF;
    #endif
}