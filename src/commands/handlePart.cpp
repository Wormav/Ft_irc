#include <Command.hpp>
#include <sys/socket.h>

void Command::handlePart(int client_fd, const std::string& line) {
    if (!users[client_fd].isAuthenticated()) {
        std::string error = ":ircserv 451 * :You have not registered\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    std::istringstream iss(line.substr(5));
    std::string channel_params;
    std::getline(iss, channel_params);
    if (channel_params[0] == ' ') channel_params = channel_params.substr(1);

    std::istringstream channel_iss(channel_params);
    std::string channel_list;
    channel_iss >> channel_list;

    std::string part_message = "Leaving";
    size_t colon_pos = channel_params.find(" :");
    if (colon_pos != std::string::npos) {
        part_message = channel_params.substr(colon_pos + 2);
    }

    std::istringstream channel_tokens(channel_list);
    std::string channel_name;
    while (std::getline(channel_tokens, channel_name, ',')) {
        if (channel_name.empty()) continue;

        if (channel_name[0] != '#') {
            channel_name = "#" + channel_name;
        }

        std::map<std::string, Channel>::iterator channel_it = channels.find(channel_name);
        if (channel_it == channels.end()) {
            std::string error = ":ircserv 403 " + users[client_fd].getNickname() + " " + channel_name + " :No such channel\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
            continue;
        }

        if (!channel_it->second.hasMember(client_fd)) {
            std::string error = ":ircserv 442 " + users[client_fd].getNickname() + " " + channel_name + " :You're not on that channel\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
            continue;
        }

        std::string part_notification = ":" + users[client_fd].getFullIdentity() + " PART " + channel_name + " :" + part_message + "\r\n";
        channel_it->second.broadcastMessage(part_notification);

        channel_it->second.removeMember(client_fd);

        if (channel_it->second.isEmpty()) {
            channels.erase(channel_it);
        }
    }
}
