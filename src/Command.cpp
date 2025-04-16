#include <Command.hpp>
#include <Server.hpp>
#include <iostream>
#include <algorithm>
#include <sys/socket.h>

Command::Command(Server* server, std::map<int, User>& users, std::map<std::string, Channel>& channels, const std::string& password)
    : server(server), users(users), channels(channels), password(password) {}

Command::~Command() {}

void Command::process(int client_fd, const std::string& line)
{
    std::istringstream iss(line);
    std::string command;
    iss >> command;

    std::transform(command.begin(), command.end(), command.begin(), ::toupper);

    if (command == "PASS")
        handlePass(client_fd, iss);
    else if (command == "NICK")
        handleNick(client_fd, iss);
    else if (command == "USER")
        handleUser(client_fd, line);
    else if (command == "JOIN")
        handleJoin(client_fd, iss);
    else if (command == "PRIVMSG")
        handlePrivmsg(client_fd, iss);
    else if (command == "PART")
        handlePart(client_fd, line);
    else if (command == "QUIT")
	{
        if (users.find(client_fd) != users.end())
            handleQuit(client_fd, line);
	}
    else if (command == "KICK")
        handleKick(client_fd, line);
    else if (command == "INVITE")
        handleInvite(client_fd, line);
    else if (command == "TOPIC")
        handleTopic(client_fd, line);
    else if (command == "MODE")
        handleMode(client_fd, line);
    else
	{
        std::string error = ":ircserv 421 " +
                           (users[client_fd].isAuthenticated() ? users[client_fd].getNickname() : std::string("*")) +
                           " " + command + " :Unknown command\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
    }
}
