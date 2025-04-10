#include <Command.hpp>
#include <sys/socket.h>

void Command::handlePrivmsg(int client_fd, std::istringstream& iss, bool isNotice) {
    const std::string command = isNotice ? "NOTICE" : "PRIVMSG";
    if (!users[client_fd].isAuthenticated()) {
        if (!isNotice) {
            std::string error = ":ircserv 451 * :You have not registered\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
        }
        return;
    }

    std::string target;
    iss >> target;

    std::string message;
    std::getline(iss, message);
    if (message.empty()) return;

    if (message[0] == ' ') message = message.substr(1);

    if (message[0] == ':') message = message.substr(1);

    std::string sender = users[client_fd].getNickname();
    std::string msg_notification = ":" + users[client_fd].getFullIdentity() + " " + command + " " + target + " :" + message + "\r\n";

    if (target[0] == '#') {
        std::map<std::string, Channel>::iterator channel_it = channels.find(target);
        if (channel_it != channels.end()) {
            if (!channel_it->second.hasMember(client_fd)) {
                if (!isNotice) {
                    std::string error = ":ircserv 442 " + sender + " " + target + " :You're not on that channel\r\n";
                    send(client_fd, error.c_str(), error.length(), 0);
                }
                return;
            }

            channel_it->second.broadcastMessage(msg_notification, client_fd);
        } else {
            if (!isNotice) {
                std::string error = ":ircserv 403 " + sender + " " + target + " :No such channel\r\n";
                send(client_fd, error.c_str(), error.length(), 0);
            }
        }
    }
    else {
        bool user_found = false;
        for (std::map<int, User>::iterator it = users.begin(); it != users.end(); ++it) {
            if (it->second.getNickname() == target) {
                user_found = true;
                send(it->first, msg_notification.c_str(), msg_notification.length(), 0);
                break;
            }
        }

        if (!user_found && !isNotice) {
            std::string error = ":ircserv 401 " + sender + " " + target + " :No such nick/channel\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
        }
    }
}
