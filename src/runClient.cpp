#include "client/headers/client.hpp"

using namespace std;

int main()
{
    try{
        Client("127.0.0.1", 0);
    }
    catch(std::exception& e) { cout << e.what() << endl;}

    return 0;
}