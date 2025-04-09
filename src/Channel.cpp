#include "Channel.hpp"
#include <sys/socket.h>
#include <unistd.h>

Channel::Channel() {}

Channel::Channel(const std::string& channelName) : name(channelName), topic("Welcome to " + channelName) {}

Channel::~Channel() {}

// Getters
const std::string& Channel::getName() const {
    return name;
}

const std::string& Channel::getTopic() const {
    return topic;
}

const std::set<int>& Channel::getMembers() const {
    return members;
}

const std::set<int>& Channel::getOperators() const {
    return operators;
}

// Setters
void Channel::setName(const std::string& channelName) {
    name = channelName;
}

void Channel::setTopic(const std::string& channelTopic) {
    topic = channelTopic;
}

// Gestion des membres
bool Channel::addMember(int client_fd) {
    return members.insert(client_fd).second;
}

bool Channel::removeMember(int client_fd) {
    removeOperator(client_fd);  // Si c'était un opérateur, on le retire aussi
    return members.erase(client_fd) > 0;
}

bool Channel::hasMember(int client_fd) const {
    return members.find(client_fd) != members.end();
}

bool Channel::isEmpty() const {
    return members.empty();
}

// Gestion des opérateurs
bool Channel::addOperator(int client_fd) {
    if (hasMember(client_fd)) {
        return operators.insert(client_fd).second;
    }
    return false;
}

bool Channel::removeOperator(int client_fd) {
    return operators.erase(client_fd) > 0;
}

bool Channel::isOperator(int client_fd) const {
    return operators.find(client_fd) != operators.end();
}

// Utilitaires
void Channel::broadcastMessage(const std::string& message, int excludeClient) const {
    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it) {
        if (*it != excludeClient) {
            send(*it, message.c_str(), message.length(), 0);
        }
    }
}
