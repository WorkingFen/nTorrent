#include "client/headers/client.hpp"

using namespace std;

void sighand(int signum)
{
    throw std::runtime_error("Received sig: " + signum);
}

int main()
{
    signal(SIGINT, sighand);

    try{
        Client client1("127.0.0.1", 0, "0.0.0.0");

        char ipAddr[15] = "127.4.0.1";
        int port = 2200;

        struct sockaddr_in x;
        x.sin_family = AF_INET;
        if( (inet_pton(AF_INET, ipAddr, &x.sin_addr)) <= 0)
            throw std::invalid_argument("Improper IPv4 address: " + std::string(ipAddr));
        x.sin_port = htons(port);

        client1.connectTo(x);
        client1.run();
        /*
        Client client2("127.1.0.1", 0, "0.0.0.0");
        Client client3("127.2.0.1", 0, "0.0.0.0");
        struct sockaddr_in x;
        socklen_t y= sizeof(x);
        getsockname(3, (struct sockaddr *) &x, &y);

        client2.connectTo(x);
        client3.connectTo(x);
        client1.run();*/
    }
    catch(std::exception& e) { std::cerr << e.what() << std::endl;}

    return 0;
}