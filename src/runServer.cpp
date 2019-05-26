#include "server/headers/server.hpp"


int main(int argc, char *argv[])
{
    try{
        server::Server srv((argc < 2)?"127.4.0.1":argv[1], (argc <= 2)?2200:atoi(argv[2]));
        srv.run_srv();
    }
    catch(std::exception& e) { std::cout << e.what() << std::endl;}

    return 0;
}
