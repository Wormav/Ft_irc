#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>
#include <set>
#include "User.hpp"
#include "Channel.hpp"

class Server
{
private:
    int port;
    std::string password;
    int server_fd;
    int epoll_fd;
    
    std::map<int, std::string> client_buffers;
    std::map<int, User> users;
    std::map<std::string, Channel> channels;
    
    // Méthodes privées pour gérer le serveur
    void setupSocket();
    void handleNewConnection();
    void handleClientData(int client_fd);
    void disconnectClient(int client_fd);
    void processCommand(int client_fd, const std::string& line);
    void sendWelcomeMessages(int client_fd, const User& user);

public:
    Server(int port, const std::string& password);
    ~Server();
    
    void run();

    // Méthodes pour gérer les commandes
    void handlePass(int client_fd, std::istringstream& iss);
    void handleNick(int client_fd, std::istringstream& iss);
    void handleUser(int client_fd, const std::string& line);
    void handleJoin(int client_fd, std::istringstream& iss);
    void handlePrivmsg(int client_fd, std::istringstream& iss, bool isNotice);
    void handlePing(int client_fd, std::istringstream& iss);
    void handlePart(int client_fd, const std::string& line);
    void handleQuit(int client_fd, std::istringstream& iss);
};

#endif