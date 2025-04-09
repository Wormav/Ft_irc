#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>
#include <sstream>
#include <map>
#include "User.hpp"
#include "Channel.hpp"

class Server;

class Command
{
private:
    Server* server;
    std::map<int, User>& users;
    std::map<std::string, Channel>& channels;
    std::string password;

public:
    Command(Server* server, std::map<int, User>& users, std::map<std::string, Channel>& channels, const std::string& password);
    ~Command();

    void process(int client_fd, const std::string& line);
    void sendWelcomeMessages(int client_fd, const User& user);

    // Méthodes pour les différentes commandes IRC
    void handlePass(int client_fd, std::istringstream& iss);
    void handleNick(int client_fd, std::istringstream& iss);
    void handleUser(int client_fd, const std::string& line);
    void handleJoin(int client_fd, std::istringstream& iss);
    void handlePrivmsg(int client_fd, std::istringstream& iss, bool isNotice);
    void handlePing(int client_fd, std::istringstream& iss);
    void handlePart(int client_fd, const std::string& line);
    void handleQuit(int client_fd, std::istringstream& iss);
    void handleKick(int client_fd, const std::string& line);
    void handleInvite(int client_fd, const std::string& line);
};

#endif
