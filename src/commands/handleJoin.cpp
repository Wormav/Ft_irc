#include <Command.hpp>
#include <sys/socket.h>

void Command::handleJoin(int client_fd, std::istringstream& iss) {
    if (!users[client_fd].isAuthenticated()) {
        std::string error = ":ircserv 451 * :You have not registered\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    std::string channel_params;
    std::getline(iss, channel_params);
    if (channel_params[0] == ' ') channel_params = channel_params.substr(1);

    std::istringstream channel_iss(channel_params);
    std::string channel_list;
    channel_iss >> channel_list;

    std::istringstream channel_tokens(channel_list);
    std::string channel_name;
    while (std::getline(channel_tokens, channel_name, ',')) {
        if (channel_name.empty()) continue;

        if (channel_name[0] != '#') {
            channel_name = "#" + channel_name;
        }

        bool isNewChannel = false;
        if (channels.find(channel_name) == channels.end()) {
            channels[channel_name] = Channel(channel_name);
            isNewChannel = true;
        }

        Channel& channel = channels[channel_name];
        if (!isNewChannel && channel.isInviteOnly() && !channel.isInvited(client_fd) && !channel.hasMember(client_fd)) {
            std::string error = ":ircserv 473 " + users[client_fd].getNickname() + " " + channel_name + " :Cannot join channel (+i)\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
            continue;
        }

        if (!isNewChannel && channel.hasKeySet()) {
            std::string key;
            std::istringstream key_iss(channel_params);
            std::string temp;
            key_iss >> temp >> key;

            if (key.empty() || key != channel.getKey()) {
                std::string error = ":ircserv 475 " + users[client_fd].getNickname() + " " + channel_name + " :Cannot join channel (+k) - bad key\r\n";
                send(client_fd, error.c_str(), error.length(), 0);
                continue;
            }
        }

        if (!isNewChannel && channel.hasUserLimitSet() && channel.getMembers().size() >= channel.getUserLimit()) {
            std::string error = ":ircserv 471 " + users[client_fd].getNickname() + " " + channel_name + " :Cannot join channel (+l) - channel is full\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
            continue;
        }

        channel.addMember(client_fd);
        channel.removeInvite(client_fd);

        if (isNewChannel) {
            channel.addOperator(client_fd);
        }

        std::string nick = users[client_fd].getNickname();

        std::string join_notification = ":" + users[client_fd].getFullIdentity() + " JOIN :" + channel_name + "\r\n";
        channel.broadcastMessage(join_notification);

        if (!channel.getTopic().empty()) {
            std::string topic_reply = ":ircserv 332 " + nick + " " + channel_name + " :" + channel.getTopic() + "\r\n";
            send(client_fd, topic_reply.c_str(), topic_reply.length(), 0);
        }

        std::string members_list;
        const std::set<int>& members = channel.getMembers();
        const std::set<int>& operators = channel.getOperators();
        for (std::set<int>::const_iterator member_it = members.begin(); member_it != members.end(); ++member_it) {
            std::string member_nick = users[*member_it].getNickname();
            if (operators.find(*member_it) != operators.end()) {
                members_list += "@" + member_nick + " ";
            } else {
                members_list += member_nick + " ";
            }
        }

        std::string names_reply = ":ircserv 353 " + nick + " = " + channel_name + " :" + members_list + "\r\n";
        std::string end_names_reply = ":ircserv 366 " + nick + " " + channel_name + " :End of /NAMES list.\r\n";

        send(client_fd, names_reply.c_str(), names_reply.length(), 0);
        send(client_fd, end_names_reply.c_str(), end_names_reply.length(), 0);
    }
}
