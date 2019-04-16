#include "server/headers/server.hpp"

int main(int argc, char *argv[])
{
    try{
        if(argc < 2) server::Server("127.4.0.1", 2200);
        else if(argc == 2) server::Server(argv[1], 2200); 
        else server::Server(argv[1], atoi(argv[2]));
    }
    catch(std::exception& e) { std::cout << e.what() << std::endl;}

    return 0;
}