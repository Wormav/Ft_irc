#include <Command.hpp>
#include <sys/socket.h>
#include <cstdlib>

static void handleModeI(Channel& channel, bool adding, std::string& modeChanges) {
    channel.setInviteOnly(adding);
    modeChanges += "i";
}

static void handleModeT(Channel& channel, bool adding, std::string& modeChanges) {
    channel.setTopicRestricted(adding);
    modeChanges += "t";
}

static void handleModeK(Channel& channel, bool adding, std::string& modeChanges,
                        std::string& modeParams, std::istringstream& iss,
                        int client_fd, const std::map<int, User>& users) {
    if (adding) {
        std::string key;
        if (!(iss >> key) || key.empty()) {
            std::string error = ":ircserv 461 " + users.at(client_fd).getNickname() + " MODE :Not enough parameters\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
            return;
        }
        channel.setKey(key);
        modeChanges += "k";
        modeParams += " " + key;
    } else {
        channel.removeKey();
        modeChanges += "k";
    }
}

static void handleModeO(Channel& channel, bool adding, std::string& modeChanges,
                        std::string& modeParams, std::istringstream& iss,
                        int client_fd, const std::map<int, User>& users) {
    std::string target_nick;
    if (!(iss >> target_nick) || target_nick.empty()) {
        std::string error = ":ircserv 461 " + users.at(client_fd).getNickname() + " MODE :Not enough parameters\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    int target_fd = -1;
    for (std::map<int, User>::const_iterator it = users.begin(); it != users.end(); ++it) {
        if (it->second.getNickname() == target_nick && channel.hasMember(it->first)) {
            target_fd = it->first;
            break;
        }
    }

    if (target_fd == -1) {
        std::string error = ":ircserv 441 " + users.at(client_fd).getNickname() + " " + target_nick + " " + channel.getName() + " :They aren't on that channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    if (adding) {
        channel.addOperator(target_fd);
    } else {
        channel.removeOperator(target_fd);
    }

    modeChanges += "o";
    modeParams += " " + target_nick;
}

static void handleModeL(Channel& channel, bool adding, std::string& modeChanges,
                        std::string& modeParams, std::istringstream& iss,
                        int client_fd, const std::map<int, User>& users) {
    if (adding) {
        std::string limitStr;
        if (!(iss >> limitStr) || limitStr.empty()) {
            std::string error = ":ircserv 461 " + users.at(client_fd).getNickname() + " MODE :Not enough parameters\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
            return;
        }

        int limitInt = atoi(limitStr.c_str());
        if (limitInt <= 0) {
            std::string error = ":ircserv 461 " + users.at(client_fd).getNickname() + " MODE :Invalid limit value\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
            return;
        }
        size_t limit = static_cast<size_t>(limitInt);
        channel.setUserLimit(limit);
        modeChanges += "l";
        modeParams += " " + limitStr;
    } else {
        channel.removeUserLimit();
        modeChanges += "l";
    }
}

void Command::handleMode(int client_fd, const std::string& line) {
    if (!users[client_fd].isAuthenticated()) {
        std::string error = ":ircserv 451 * :You have not registered\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    std::istringstream iss(line.substr(5));
    std::string target, modes;
    iss >> target;

    if (target.empty()) {
        std::string error = ":ircserv 461 " + users[client_fd].getNickname() + " MODE :Not enough parameters\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    if (target[0] == '#' && channels.find(target) == channels.end()) {
        std::string error = ":ircserv 403 " + users[client_fd].getNickname() + " " + target + " :No such channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    if (target[0] != '#') {
        std::string error = ":ircserv 502 " + users[client_fd].getNickname() + " :Cannot change mode for other users\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    Channel& channel = channels[target];

    if (!(iss >> modes)) {
        std::string mode_response = ":ircserv 324 " + users[client_fd].getNickname() + " " + target + " " + channel.getModeString() + "\r\n";
        send(client_fd, mode_response.c_str(), mode_response.length(), 0);
        return;
    }

    if (!channel.hasMember(client_fd)) {
        std::string error = ":ircserv 442 " + users[client_fd].getNickname() + " " + target + " :You're not on that channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    if (!channel.isOperator(client_fd)) {
        std::string error = ":ircserv 482 " + users[client_fd].getNickname() + " " + target + " :You're not channel operator\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    bool adding = true;
    std::string modeChanges = "+";
    std::string modeParams = "";

    for (size_t i = 0; i < modes.length(); ++i) {
        char c = modes[i];

        if (c == '+') {
            adding = true;
            modeChanges = "+";
        }
        else if (c == '-') {
            adding = false;
            modeChanges = "-";
        }
        else if (c == 'i') {
            handleModeI(channel, adding, modeChanges);
        }
        else if (c == 't') {
            handleModeT(channel, adding, modeChanges);
        }
        else if (c == 'k') {
            handleModeK(channel, adding, modeChanges, modeParams, iss, client_fd, users);
        }
        else if (c == 'o') {
            handleModeO(channel, adding, modeChanges, modeParams, iss, client_fd, users);
        }
        else if (c == 'l') {
            handleModeL(channel, adding, modeChanges, modeParams, iss, client_fd, users);
        }
    }

    if (modeChanges.length() > 1) {
        std::string mode_notification = ":" + users[client_fd].getFullIdentity() + " MODE " + target + " " + modeChanges + modeParams + "\r\n";
        channel.broadcastMessage(mode_notification);
    }
}
