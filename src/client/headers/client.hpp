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
#include <memory>
#include "../../message/message.hpp"

class Client
{
    private:
        class ConsoleInterface;
        class FileManager;
        int pieceSize = 10;

        int sockFd, port, clientSocketsNum, serverSocketsNum, maxFd;        // listen socket; przydzielony port efemeryczny; liczba socketów pobierających/wysyłających dane (nie licząc komunikacji z serwerem)
        struct sockaddr_in self, server;
        fd_set ready;
        struct timeval to;

        int mainServerSocket = -1;
        std::list<int> clientSockets;                             // lista z socketami pełniącymi role leechów/peerów
        std::list<int> serverSockets;                             // lista z socketami pełniącymi role seederów/peerów    

        std::thread input;
        msg::MessageManager msg_manager;
        std::unique_ptr<ConsoleInterface> console;
        std::unique_ptr<FileManager> fileManager;

        std::thread signal_thread;              
        sigset_t signal_set;					// do ustawienia sigmask
        bool interrupted_flag = false;			// sygnalizuje użycie Ctrl+C
        bool run_stop_flag = false;             // koniec pętli run

        void signal_waiter();					// obsługa siginta na fredach
        
        void setSigmask();
        void prepareSockaddrStruct(struct sockaddr_in& x, const char ipAddr[15], const int& port);

        void handleMessagesfromServer(); 
        void handleMessages();
        void getUserCommands();
        void handleCommands();

        void turnOff();                                               // metoda kończąca wszystkie połączenia
        void setFileDescrMask();                                      // metoda ustawiająca maskę deskryptorów plików

    public:
        Client(const char ipAddr[15], const int& port, const char serverIpAddr[15], const int& serverPort=2200);         // tworzy socketa, który będzie nasłuchiwał
        ~Client();

        void connectTo(const struct sockaddr_in &server);             // łączy się z klientem o podanym adresie (serwer powinien przesyłać gotową strukturę do klienta) lub z serwerem torrent

        const struct sockaddr_in& getServer() const;

        void shareFile(int socket, std::string directory, std::string fname);
        void shareFiles();
        void sendFileInfo(int socket, std::string directory, std::string filename);
        void sendFilesInfo();
        void sendDeleteBlock(int socket, std::string fileName, int blockIndex);
        void sendAskForFile(int socket, std::string fileName);
        void sendHaveBlock(int socket, std::string fileName, int blockIndex, std::string hash);
        void sendAskForBlock(int socket, std::string fileName, std::vector<int> blockList);
        void sendBadBlockHash(int socket, std::string fileName, int blockIndex, std::string seederAdress);


        void run();                                                   // pętla z selectem



};

#endif
