#include <Channel.hpp>
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <cstdlib>
#include <cerrno>

Channel::Channel() : inviteOnly(false), topicRestricted(true), hasUserLimit(false), hasKey(false), userLimit(0) {}

Channel::Channel(const std::string& channelName) : name(channelName), topic("Welcome to " + channelName),
    inviteOnly(false), topicRestricted(true), hasUserLimit(false), hasKey(false), userLimit(0) {}

Channel::~Channel() {}

const std::string& Channel::getName() const
{
    return name;
}

const std::string& Channel::getTopic() const
{
    return topic;
}

const std::set<int>& Channel::getMembers() const
{
    return members;
}

const std::set<int>& Channel::getOperators() const
{
    return operators;
}

void Channel::setName(const std::string& channelName)
{
    name = channelName;
}

void Channel::setTopic(const std::string& channelTopic)
{
    topic = channelTopic;
}

bool Channel::addMember(int client_fd)
{
    return members.insert(client_fd).second;
}

bool Channel::removeMember(int client_fd)
{
    if (members.find(client_fd) == members.end())
        return false;

    removeOperator(client_fd);

    return members.erase(client_fd) > 0;
}

bool Channel::hasMember(int client_fd) const
{
    return members.find(client_fd) != members.end();
}

bool Channel::isEmpty() const
{
    return members.empty();
}

bool Channel::addOperator(int client_fd)
{
    if (hasMember(client_fd))
        return operators.insert(client_fd).second;
    return false;
}

bool Channel::removeOperator(int client_fd)
{
    return operators.erase(client_fd) > 0;
}

bool Channel::isOperator(int client_fd) const
{
    return operators.find(client_fd) != operators.end();
}

bool Channel::addInvite(int client_fd)
{
    return invited.insert(client_fd).second;
}

bool Channel::removeInvite(int client_fd)
{
    return invited.erase(client_fd) > 0;
}

bool Channel::isInvited(int client_fd) const
{
    return invited.find(client_fd) != invited.end();
}

bool Channel::isInviteOnly() const
{
    return inviteOnly;
}

bool Channel::isTopicRestricted() const
{
    return topicRestricted;
}

bool Channel::hasKeySet() const
{
    return hasKey;
}

bool Channel::hasUserLimitSet() const
{
    return hasUserLimit;
}

const std::string& Channel::getKey() const
{
    return key;
}

size_t Channel::getUserLimit() const
{
    return userLimit;
}

void Channel::setInviteOnly(bool value)
{
    inviteOnly = value;
}

void Channel::setTopicRestricted(bool value)
{
    topicRestricted = value;
}

void Channel::setKey(const std::string& newKey)
{
    key = newKey;
    hasKey = true;
}

void Channel::removeKey()
{
    key.clear();
    hasKey = false;
}

void Channel::setUserLimit(size_t limit)
{
    userLimit = limit;
    hasUserLimit = true;
}

void Channel::removeUserLimit()
{
    userLimit = 0;
    hasUserLimit = false;
}

std::string Channel::getModeString() const
{
    std::string modes = "+";
    std::string params = "";

    if (inviteOnly) modes += "i";
    if (topicRestricted) modes += "t";

    if (hasKey)
	{
        modes += "k";
        params += " " + key;
    }

    if (hasUserLimit)
	{
        modes += "l";
        std::stringstream ss;
        ss << userLimit;
        params += " " + ss.str();
    }

    return modes + params;
}

void Channel::broadcastMessage(const std::string& message, int excludeClient) const
{
    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
	{
        if (*it != excludeClient)
		{
            if (*it > 0)
                send(*it, message.c_str(), message.length(), MSG_NOSIGNAL);
        }
    }
}
