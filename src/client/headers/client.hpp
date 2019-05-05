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
#include <signal.h>
#include <thread>
#include <mutex>

class Client{
    int sockFd, port, clientSocketsNum, serverSocketsNum, maxFd;        // listen socket; przydzielony port efemeryczny; liczba socketów pobierających/wysyłających dane (nie licząc komunikacji z serwerem)
    char fileDir[1000];
    struct sockaddr_in self, server;
	fd_set ready;
	struct timeval to;
	std::list<int> clientSockets;                             // lista z socketami pełniącymi role leechów/peerów
    std::list<int> serverSockets;                             // lista z socketami pełniącymi role seederów/peerów                        
    enum class State {down, up, connected};                       // stan, w jakim znajduje się użytkownik (determinuje obsługę i/o)
    State state;
    std::thread input;
    std::mutex input_lock;// = PTHREAD_MUTEX_INITIALIZER;
    int command = 0;
    void input_thread();  
    //  ?????? InputHandler inputHandler;                              // obsługuje input od użytkownika, może wywoływać np. connect
   



    void signal_waiter();					// obsługa siginta na fredach
    void setSigmask();
    sigset_t signal_set;					// do ustawienia sigmask
    bool interrupted_flag = false;				// sygnalizuje użycie Ctrl+C

    /*
        funkcje do obsługi msg
    */
    void prepareSockaddrStruct(struct sockaddr_in& x, const char ipAddr[15], const int& port);
    public:
    Client(const char ipAddr[15], const int& port, const char serverIpAddr[15], const int& serverPort=2200);         // tworzy socketa, który będzie nasłuchiwał
    ~Client();
    void connectTo(struct sockaddr_in &server);             // łączy się z klientem o podanym adresie (serwer powinien przesyłać gotową strukturę do klienta) lub z serwerem torrent
    void turnOff();                                         // metoda kończąca wszystkie połączenia
    void run();                                             // pętla z selectem

    //void handleMessage(Message msg);
    //void disconnectFrom(int socket);

    void registerSignalHandler(void (*handler)(int));
};

#endif
