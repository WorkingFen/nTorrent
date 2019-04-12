#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <list>
#include <iostream>

class Client{
    int sockFd, port, iSockets, oSockets;                    // listen socket; przydzielony port efemeryczny; liczba socketów pobierających/wysyłających dane (nie licząc komunikacji z serwerem)
    char fileDir[1000];
    struct sockaddr_in self, server;
	fd_set ready;
	struct timeval to;
	std::list<int> commSockets;                             // lista z socketami do komunikacji (należy zadbać, aby komunikaty z/do serwera były łatwe do odróżnienia od tych wysyłanych omiędzy klientami)
    enum class State {down, up, connected};                       // stan, w jakim znajduje się użytkownik (determinuje obsługę i/o)
    State state;              
    //  ?????? InputHandler inputHandler;                              // obsługuje input od użytkownika, może wywoływać np. connect

    /*
        funkcje do obsługi msg
    */

    public:
    Client(const char ipAddr[15], const int& port);         // tworzy socketa, który będzie nasłuchiwał
    ~Client();
    void connectTo(struct sockaddr_in &server);             // łączy się z klientem o podanym adresie (serwer powinien przesyłać gotową strukturę do klienta) lub z serwerem torrent
    void turnOff();                                         // metoda kończąca wszystkie połączenia
    void run();                                             // pętla z selectem
};

#endif