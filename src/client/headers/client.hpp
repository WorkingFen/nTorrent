#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <list>
#include <iostream>
#include <signal.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "consoleInterface.hpp"
#include "../../message/message.hpp"

class ConsoleInterface;

typedef std::unique_ptr<ConsoleInterface> ConsoleInterfacePtr;

class Client
{
    private:
        static const int pieceSize = 10;

        int sockFd, port, clientSocketsNum, serverSocketsNum, maxFd;        // listen socket; przydzielony port efemeryczny; liczba socketów pobierających/wysyłających dane (nie licząc komunikacji z serwerem)
        int endFlag;                            // sygnalizuje, że użytkownik wybrał opcję zakończenia programu lub wystąpił wyjątek przy inputHandler 
        struct sockaddr_in self, server;
        fd_set ready;
        struct timeval to;
        std::list<int> clientSockets;                             // lista z socketami pełniącymi role leechów/peerów
        std::list<int> serverSockets;                             // lista z socketami pełniącymi role seederów/peerów    

        std::thread input;
        std::mutex commandLock, inputLock;// = PTHREAD_MUTEX_INITIALIZER;
        std::condition_variable condition;
        int command = 0;
        void input_thread();
        msg::MessageManager msg_manager;
        ConsoleInterfacePtr console;
    
        void signal_waiter();					// obsługa siginta na fredach
        void setSigmask();
        std::thread signal_thread;              
        sigset_t signal_set;					// do ustawienia sigmask
        bool interrupted_flag = false;			// sygnalizuje użycie Ctrl+C
        bool run_stop_flag = false;             // koniec pętli run

        void prepareSockaddrStruct(struct sockaddr_in& x, const char ipAddr[15], const int& port);

    public:
        Client(const char ipAddr[15], const int& port, const char serverIpAddr[15], const int& serverPort=2200);         // tworzy socketa, który będzie nasłuchiwał
        ~Client();
        void connectTo(const struct sockaddr_in &server);             // łączy się z klientem o podanym adresie (serwer powinien przesyłać gotową strukturę do klienta) lub z serwerem torrent
        void turnOff();                                         // metoda kończąca wszystkie połączenia
        void handleMessages();
        void handleCommands();
        void sendFilesInfo();
        void run();                                                // pętla z selectem
        //void handleMessage(Message msg);
        //void disconnectFrom(int socket);
        void registerSignalHandler(void (*handler)(int));
        const struct sockaddr_in& getServer() const;
        void setConsoleInterface(ConsoleInterfacePtr& x);
};

#endif
