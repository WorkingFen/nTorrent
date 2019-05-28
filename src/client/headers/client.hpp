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
        int pieceSize = 400000;

        int sockFd, port, maxFd;        // listen socket; przydzielony port efemeryczny; liczba socketów pobierających/wysyłających dane (nie licząc komunikacji z serwerem)
        struct sockaddr_in self, server;
        fd_set ready;
        struct timeval to;

        int mainServerSocket = -1;
        struct FileSocket
        {
            int sockFd;

            std::string filename = std::string();
            int blockIndex = -1;

            std::string hash;
            std::time_t last_activity = std::time(0);

            FileSocket() {}
            FileSocket(int i): sockFd(i) {}
        };

        std::list<FileSocket> seederSockets;                             // lista z socketami pełniącymi role leechów/peerów
        std::list<FileSocket> leecherSockets;                             // lista z socketami pełniącymi role seederów/peerów  
        std::time_t timeout = 20;  

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
        void prepareSockaddrStruct(struct sockaddr_in& x, const int ipAddr, const int& port);

        void handleMessagesfromServer(); 
        void handleMessagesfromLeechers();
        void handleMessagesfromSeeders();

        void handleServerFileInfo(msg::Message msg);        // handler dla 201
        void handleServerBlockInfo(msg::Message msg);       // handler dla 202

        void handleSeederFile(FileSocket &s, msg::Message &msg);

        void getUserCommands();
        void handleCommands();

        void disconnect();
        void turnOff();                                               // metoda kończąca wszystkie połączenia
        void setFileDescrMask();                                      // metoda ustawiająca maskę deskryptorów plików

    public:
        Client(const char ipAddr[15], const int& port, const char serverIpAddr[15], const int& serverPort=2200);         // tworzy socketa, który będzie nasłuchiwał
        ~Client();

        void connectTo(const struct sockaddr_in &server);             // łączy się z klientem o podanym adresie (serwer powinien przesyłać gotową strukturę do klienta) lub z serwerem torrent

        const struct sockaddr_in& getServer() const;

        void shareFile(std::string directory, std::string fname);   //to wszystko raczej prywatne, a nie publiczne
        void shareFiles();
        void sendFileInfo(int socket, std::string directory, std::string filename);
        void sendFilesInfo();
        void sendDeleteBlock(int socket, std::string fileName, int blockIndex);
        void sendDeleteBlocks(std::string fileName);
        void sendDeleteFile(std::string fileName);
        void sendAskForFile(std::string fileName);
        void sendHaveBlock(int socket, std::string fileName, int blockIndex, std::string hash);
        void sendAskForBlock(int socket, std::string fileName, std::vector<int> blockList);
        void sendBadBlockHash(int socket, std::string fileName, int blockIndex, std::string seederAdress);
        void listServerFiles();
        void leechFile(const int ipAddr, int port, std::string filename, int blockIndex, std::string hash);
        void seedFile(int socket, std::string filename, int blockIndex);

        void sendBadBlockHash(int socket, std::string fileName, int blockIndex, int seederAddress, int seederPort);

        void run();                                                   // pętla z selectem

};

class ClientException : public std::exception
{
    const std::string info;

    public:
    ClientException(const std::string& msg);
    const char* what() const throw();
};

#endif
