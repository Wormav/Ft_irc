#include <Command.hpp>
#include <sys/socket.h>


void Command::sendWelcomeMessages(int client_fd, const User& user)
{
    std::string welcome = ":ircserv 001 " + user.getNickname() + " :Welcome to the IRC Network " + user.getFullIdentity() + "\r\n";
    std::string yourhost = ":ircserv 002 " + user.getNickname() + " :Your host is ircserv, running version 1.0\r\n";
    std::string created = ":ircserv 003 " + user.getNickname() + " :This server was created Apr 2025\r\n";
    std::string myinfo = ":ircserv 004 " + user.getNickname() + " ircserv 1.0 o o\r\n";

    send(client_fd, welcome.c_str(), welcome.length(), 0);
    send(client_fd, yourhost.c_str(), yourhost.length(), 0);
    send(client_fd, created.c_str(), created.length(), 0);
    send(client_fd, myinfo.c_str(), myinfo.length(), 0);
}
