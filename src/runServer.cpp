#include "server/headers/server.hpp"

int main(int argc, char *argv[])
{
    try{
        if(argc < 2) server::Server(6969);
        else server::Server(atoi(argv[1]));
    }
    catch(std::exception& e) { std::cout << e.what() << std::endl;}

    return 0;
}