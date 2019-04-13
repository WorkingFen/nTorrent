#include "client/headers/client.hpp"

using namespace std;

int main()
{
    try{
        Client client1("127.0.0.1", 0);
        
        /*
        Client client2("127.1.0.1", 0);
        Client client3("127.2.0.1", 0);
        struct sockaddr_in x;
        socklen_t y= sizeof(x);
        getsockname(3, (struct sockaddr *) &x, &y);
        client2.connectTo(x);
        client3.connectTo(x);
        client1.run();
        */
        
    }
    catch(std::exception& e) { cout << e.what() << endl;}

    return 0;
}