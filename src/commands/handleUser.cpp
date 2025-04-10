#include <Command.hpp>
#include <sys/socket.h>

void Command::handleUser(int client_fd, const std::string& line) {
    if (!users[client_fd].isPasswordVerified()) {
        std::string error = ":ircserv 464 * :Password required before registration\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    std::string params = line.substr(5);
    std::istringstream params_iss(params);
    std::string username, mode, unused;
    params_iss >> username >> mode >> unused;

    std::string realname;
    if (std::getline(params_iss, realname) && !realname.empty()) {
        if (realname[0] == ' ') realname = realname.substr(1);
        if (realname[0] == ':') realname = realname.substr(1);
    }

    users[client_fd].setUsername(username);
    users[client_fd].setRealname(realname);

    if (!users[client_fd].getNickname().empty() && !users[client_fd].isAuthenticated()) {
        users[client_fd].setAuthenticated(true);
        sendWelcomeMessages(client_fd, users[client_fd]);
    }
}
