#include <Command.hpp>
#include <Server.hpp>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>

Command::Command(Server* server, std::map<int, User>& users, std::map<std::string, Channel>& channels, const std::string& password)
    : server(server), users(users), channels(channels), password(password) {}

Command::~Command() {}

void Command::process(int client_fd, const std::string& line) {
    std::istringstream iss(line);
    std::string command;
    iss >> command;

    std::transform(command.begin(), command.end(), command.begin(), ::toupper);

    if (command == "PASS") {
        handlePass(client_fd, iss);
    }
    else if (command == "NICK") {
        handleNick(client_fd, iss);
    }
    else if (command == "USER") {
        handleUser(client_fd, line);
    }
    else if (command == "JOIN") {
        handleJoin(client_fd, iss);
    }
    else if (command == "PRIVMSG") {
        handlePrivmsg(client_fd, iss, false);
    }
    else if (command == "NOTICE") {
        handlePrivmsg(client_fd, iss, true);
    }
    else if (command == "PING") {
        handlePing(client_fd, iss);
    }
    else if (command == "PART") {
        handlePart(client_fd, line);
    }
    else if (command == "QUIT") {
        handleQuit(client_fd, iss);
    }
    else if (command == "KICK") {
        handleKick(client_fd, line);
    }
    else if (command == "INVITE") {
        handleInvite(client_fd, line);
    }
    else if (command == "TOPIC") {
        handleTopic(client_fd, line);
    }
    else {
        std::string error = ":ircserv 421 " +
                           (users[client_fd].isAuthenticated() ? users[client_fd].getNickname() : std::string("*")) +
                           " " + command + " :Unknown command\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
    }
}

void Command::sendWelcomeMessages(int client_fd, const User& user) {
    std::string welcome = ":ircserv 001 " + user.getNickname() + " :Welcome to the IRC Network " + user.getFullIdentity() + "\r\n";
    std::string yourhost = ":ircserv 002 " + user.getNickname() + " :Your host is ircserv, running version 1.0\r\n";
    std::string created = ":ircserv 003 " + user.getNickname() + " :This server was created Apr 2025\r\n";
    std::string myinfo = ":ircserv 004 " + user.getNickname() + " ircserv 1.0 o o\r\n";

    send(client_fd, welcome.c_str(), welcome.length(), 0);
    send(client_fd, yourhost.c_str(), yourhost.length(), 0);
    send(client_fd, created.c_str(), created.length(), 0);
    send(client_fd, myinfo.c_str(), myinfo.length(), 0);
}

void Command::handlePass(int client_fd, std::istringstream& iss) {
    std::string pass;
    if (std::getline(iss, pass) && !pass.empty()) {
        if (pass[0] == ' ') pass = pass.substr(1);
        size_t crlf = pass.find_first_of("\r\n");
        if (crlf != std::string::npos) {
            pass = pass.substr(0, crlf);
        }

        if (pass == password) {
            users[client_fd].setPasswordVerified(true);
        } else {
            std::string error = ":ircserv 464 * :Password incorrect\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
        }
    }
}

void Command::handleNick(int client_fd, std::istringstream& iss) {
    std::string nickname;
    if (std::getline(iss, nickname) && !nickname.empty()) {
        if (nickname[0] == ' ') nickname = nickname.substr(1);

        size_t crlf = nickname.find_first_of("\r\n");
        if (crlf != std::string::npos) {
            nickname = nickname.substr(0, crlf);
        }

        if (nickname.empty() || nickname.find(' ') != std::string::npos) {
            std::string error = ":ircserv 432 * :Erroneous nickname\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
        } else {
            bool nickname_in_use = false;
            for (std::map<int, User>::iterator it = users.begin(); it != users.end(); ++it) {
                if (it->first != client_fd && it->second.getNickname() == nickname) {
                    nickname_in_use = true;
                    break;
                }
            }

            if (nickname_in_use) {
                std::string error = ":ircserv 433 * " + nickname + " :Nickname is already in use\r\n";
                send(client_fd, error.c_str(), error.length(), 0);
            } else {
                std::string old_nick = users[client_fd].getNickname();
                users[client_fd].setNickname(nickname);

                std::string response;
                if (old_nick.empty()) {
                    response = ":" + nickname + " NICK :" + nickname + "\r\n";
                } else {
                    response = ":" + old_nick + "!~" + users[client_fd].getUsername() + "@localhost NICK :" + nickname + "\r\n";
                }
                send(client_fd, response.c_str(), response.length(), 0);

                if (!users[client_fd].getUsername().empty() &&
                    users[client_fd].isPasswordVerified() &&
                    !users[client_fd].isAuthenticated()) {
                    users[client_fd].setAuthenticated(true);
                    sendWelcomeMessages(client_fd, users[client_fd]);
                }
            }
        }
    }
}

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

        // Vérifier si l'utilisateur est invité ou si c'est un nouveau canal
        if (!isNewChannel && !channels[channel_name].isInvited(client_fd) && !channels[channel_name].hasMember(client_fd)) {
            std::string error = ":ircserv 473 " + users[client_fd].getNickname() + " " + channel_name + " :Cannot join channel (+i)\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
            continue;
        }

        // Ajouter l'utilisateur au canal et retirer l'invitation
        channels[channel_name].addMember(client_fd);
        channels[channel_name].removeInvite(client_fd);

        // Si c'est un nouveau canal, le premier utilisateur devient opérateur
        if (isNewChannel) {
            channels[channel_name].addOperator(client_fd);
        }

        std::string nick = users[client_fd].getNickname();

        std::string join_notification = ":" + users[client_fd].getFullIdentity() + " JOIN :" + channel_name + "\r\n";
        channels[channel_name].broadcastMessage(join_notification);

        if (!channels[channel_name].getTopic().empty()) {
            std::string topic_reply = ":ircserv 332 " + nick + " " + channel_name + " :" + channels[channel_name].getTopic() + "\r\n";
            send(client_fd, topic_reply.c_str(), topic_reply.length(), 0);
        }

        std::string members_list;
        const std::set<int>& members = channels[channel_name].getMembers();
        const std::set<int>& operators = channels[channel_name].getOperators();
        for (std::set<int>::const_iterator member_it = members.begin(); member_it != members.end(); ++member_it) {
            std::string member_nick = users[*member_it].getNickname();
            // Ajouter le symbole @ pour les opérateurs
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

void Command::handlePing(int client_fd, std::istringstream& iss) {
    std::string token;
    iss >> token;
    std::string response = "PONG " + token + "\r\n";
    send(client_fd, response.c_str(), response.length(), 0);
}

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

void Command::handleQuit(int client_fd, std::istringstream& iss) {
    std::string quit_message = "Quit";

    std::string rest;
    if (std::getline(iss, rest) && !rest.empty()) {
        size_t colon_pos = rest.find(':');
        if (colon_pos != std::string::npos) {
            quit_message = rest.substr(colon_pos + 1);
        } else if (rest[0] == ' ') {
            quit_message = rest.substr(1);
        }
    }

    std::string quit_response = "ERROR :Closing Link: localhost (" + quit_message + ")\r\n";
    send(client_fd, quit_response.c_str(), quit_response.length(), 0);

    server->disconnectClient(client_fd);
}

void Command::handleKick(int client_fd, const std::string& line) {
    if (!users[client_fd].isAuthenticated()) {
        std::string error = ":ircserv 451 * :You have not registered\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    std::istringstream iss(line.substr(5)); // Ignorer "KICK "
    std::string channel_name, target_nick;

    iss >> channel_name >> target_nick;

    if (channel_name.empty() || target_nick.empty()) {
        std::string error = ":ircserv 461 " + users[client_fd].getNickname() + " KICK :Not enough parameters\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    if (channel_name[0] != '#') {
        channel_name = "#" + channel_name;
    }

    // Extraire le message de kick (facultatif)
    std::string kick_message = users[client_fd].getNickname();
    std::string rest;
    std::getline(iss, rest);

    size_t colon_pos = rest.find(':');
    if (colon_pos != std::string::npos) {
        kick_message = rest.substr(colon_pos + 1);
    }

    // Vérifier si le canal existe
    std::map<std::string, Channel>::iterator channel_it = channels.find(channel_name);
    if (channel_it == channels.end()) {
        std::string error = ":ircserv 403 " + users[client_fd].getNickname() + " " + channel_name + " :No such channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    // Vérifier si l'utilisateur est sur le canal
    if (!channel_it->second.hasMember(client_fd)) {
        std::string error = ":ircserv 442 " + users[client_fd].getNickname() + " " + channel_name + " :You're not on that channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    // Vérifier si l'utilisateur est un opérateur
    if (!channel_it->second.isOperator(client_fd)) {
        std::string error = ":ircserv 482 " + users[client_fd].getNickname() + " " + channel_name + " :You're not channel operator\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    // Trouver l'utilisateur cible
    int target_fd = -1;
    for (std::map<int, User>::iterator it = users.begin(); it != users.end(); ++it) {
        if (it->second.getNickname() == target_nick) {
            target_fd = it->first;
            break;
        }
    }

    if (target_fd == -1 || !channel_it->second.hasMember(target_fd)) {
        std::string error = ":ircserv 441 " + users[client_fd].getNickname() + " " + target_nick + " " + channel_name + " :They aren't on that channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    // Envoyer le message de kick à tous les membres du canal
    std::string kick_notification = ":" + users[client_fd].getFullIdentity() + " KICK " + channel_name + " " + target_nick + " :" + kick_message + "\r\n";
    channel_it->second.broadcastMessage(kick_notification);

    // Retirer l'utilisateur cible du canal
    channel_it->second.removeMember(target_fd);
}

void Command::handleInvite(int client_fd, const std::string& line) {
    if (!users[client_fd].isAuthenticated()) {
        std::string error = ":ircserv 451 * :You have not registered\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    std::istringstream iss(line.substr(7)); // Ignorer "INVITE "
    std::string nickname, channel_name;

    iss >> nickname >> channel_name;

    if (nickname.empty() || channel_name.empty()) {
        std::string error = ":ircserv 461 " + users[client_fd].getNickname() + " INVITE :Not enough parameters\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    if (channel_name[0] != '#') {
        channel_name = "#" + channel_name;
    }

    // Vérifier si le canal existe
    std::map<std::string, Channel>::iterator channel_it = channels.find(channel_name);
    if (channel_it == channels.end()) {
        std::string error = ":ircserv 403 " + users[client_fd].getNickname() + " " + channel_name + " :No such channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    // Vérifier si l'utilisateur est sur le canal
    if (!channel_it->second.hasMember(client_fd)) {
        std::string error = ":ircserv 442 " + users[client_fd].getNickname() + " " + channel_name + " :You're not on that channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    // Vérifier si l'utilisateur est un opérateur
    if (!channel_it->second.isOperator(client_fd)) {
        std::string error = ":ircserv 482 " + users[client_fd].getNickname() + " " + channel_name + " :You're not channel operator\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    // Trouver l'utilisateur cible
    int target_fd = -1;
    for (std::map<int, User>::iterator it = users.begin(); it != users.end(); ++it) {
        if (it->second.getNickname() == nickname) {
            target_fd = it->first;
            break;
        }
    }

    if (target_fd == -1) {
        std::string error = ":ircserv 401 " + users[client_fd].getNickname() + " " + nickname + " :No such nick/channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    // Vérifier si l'utilisateur cible est déjà dans le canal
    if (channel_it->second.hasMember(target_fd)) {
        std::string error = ":ircserv 443 " + users[client_fd].getNickname() + " " + nickname + " " + channel_name + " :is already on channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    // Ajouter l'invitation
    channel_it->second.addInvite(target_fd);

    // Envoi de la notification d'invitation à l'utilisateur cible
    std::string invite_notification = ":" + users[client_fd].getFullIdentity() + " INVITE " + nickname + " :" + channel_name + "\r\n";
    send(target_fd, invite_notification.c_str(), invite_notification.length(), 0);

    // Confirmation à l'utilisateur qui a envoyé l'invitation
    std::string invite_confirm = ":ircserv 341 " + users[client_fd].getNickname() + " " + nickname + " " + channel_name + "\r\n";
    send(client_fd, invite_confirm.c_str(), invite_confirm.length(), 0);
}

void Command::handleTopic(int client_fd, const std::string& line) {
    if (!users[client_fd].isAuthenticated()) {
        std::string error = ":ircserv 451 * :You have not registered\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    std::istringstream iss(line.substr(6)); // Ignorer "TOPIC "
    std::string channel_name;
    iss >> channel_name;

    if (channel_name.empty()) {
        std::string error = ":ircserv 461 " + users[client_fd].getNickname() + " TOPIC :Not enough parameters\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    if (channel_name[0] != '#') {
        channel_name = "#" + channel_name;
    }

    // Vérifier si le canal existe
    std::map<std::string, Channel>::iterator channel_it = channels.find(channel_name);
    if (channel_it == channels.end()) {
        std::string error = ":ircserv 403 " + users[client_fd].getNickname() + " " + channel_name + " :No such channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    // Vérifier si l'utilisateur est sur le canal
    if (!channel_it->second.hasMember(client_fd)) {
        std::string error = ":ircserv 442 " + users[client_fd].getNickname() + " " + channel_name + " :You're not on that channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    // Reste de la ligne après le nom du canal
    std::string rest;
    std::getline(iss, rest);

    // Si aucun nouveau topic n'est fourni, afficher le topic actuel
    if (rest.empty() || rest == " ") {
        if (channel_it->second.getTopic().empty()) {
            // Pas de topic défini
            std::string no_topic = ":ircserv 331 " + users[client_fd].getNickname() + " " + channel_name + " :No topic is set\r\n";
            send(client_fd, no_topic.c_str(), no_topic.length(), 0);
        } else {
            // Afficher le topic actuel
            std::string topic_reply = ":ircserv 332 " + users[client_fd].getNickname() + " " + channel_name + " :" + channel_it->second.getTopic() + "\r\n";
            send(client_fd, topic_reply.c_str(), topic_reply.length(), 0);
        }
        return;
    }

    // Si un nouveau topic est fourni, vérifier que l'utilisateur est opérateur
    if (!channel_it->second.isOperator(client_fd)) {
        std::string error = ":ircserv 482 " + users[client_fd].getNickname() + " " + channel_name + " :You're not channel operator\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    // Extraire le nouveau topic
    std::string new_topic;
    size_t colon_pos = rest.find(':');
    if (colon_pos != std::string::npos) {
        new_topic = rest.substr(colon_pos + 1);
    } else if (rest[0] == ' ') {
        new_topic = rest.substr(1);
    }

    // Mettre à jour le topic
    channel_it->second.setTopic(new_topic);

    // Notifier tous les membres du canal du changement de topic
    std::string topic_notification = ":" + users[client_fd].getFullIdentity() + " TOPIC " + channel_name + " :" + new_topic + "\r\n";
    channel_it->second.broadcastMessage(topic_notification);
}
