#include "client/headers/client.hpp"

using namespace std;

int main(int argc, char* argv[])
{

    try
    {
        Client client((argc < 2) ? "127.0.0.1" : argv[1], 0, "127.4.0.1", 2200);
        client.run();
    }
    catch(std::exception& e) { std::cerr << e.what() << std::endl;}

    return 0;
}

