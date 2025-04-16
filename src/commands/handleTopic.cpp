#include <Command.hpp>
#include <sys/socket.h>

void Command::handleTopic(int client_fd, const std::string& line)
{
    if (!users[client_fd].isAuthenticated())
	{
        std::string error = ":ircserv 451 * :You have not registered\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    std::istringstream iss;
    if (line.length() > 6)
        iss.str(line.substr(6));
    std::string channel_name;
    iss >> channel_name;

    if (channel_name.empty())
	{
        std::string error = ":ircserv 461 " + users[client_fd].getNickname() + " TOPIC :Not enough parameters\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    if (channel_name[0] != '#')
        channel_name = "#" + channel_name;

    std::map<std::string, Channel>::iterator channel_it = channels.find(channel_name);
    if (channel_it == channels.end())
	{
        std::string error = ":ircserv 403 " + users[client_fd].getNickname() + " " + channel_name + " :No such channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    if (!channel_it->second.hasMember(client_fd))
	{
        std::string error = ":ircserv 442 " + users[client_fd].getNickname() + " " + channel_name + " :You're not on that channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    std::string rest;
    std::getline(iss, rest);

    if (rest.empty() || rest == " ")
	{
        if (channel_it->second.getTopic().empty())
		{
            std::string no_topic = ":ircserv 331 " + users[client_fd].getNickname() + " " + channel_name + " :No topic is set\r\n";
            send(client_fd, no_topic.c_str(), no_topic.length(), 0);
        }
		else
		{
            std::string topic_reply = ":ircserv 332 " + users[client_fd].getNickname() + " " + channel_name + " :" + channel_it->second.getTopic() + "\r\n";
            send(client_fd, topic_reply.c_str(), topic_reply.length(), 0);
        }
        return;
    }

    if (channel_it->second.isTopicRestricted() && !channel_it->second.isOperator(client_fd))
	{
        std::string error = ":ircserv 482 " + users[client_fd].getNickname() + " " + channel_name + " :You're not channel operator\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    std::string new_topic;
    size_t colon_pos = rest.find(':');
    if (colon_pos != std::string::npos)
        new_topic = rest.substr(colon_pos + 1);
    else if (rest[0] == ' ')
        new_topic = rest.substr(1);

    channel_it->second.setTopic(new_topic);

    std::string topic_notification = ":" + users[client_fd].getFullIdentity() + " TOPIC " + channel_name + " :" + new_topic + "\r\n";
    channel_it->second.broadcastMessage(topic_notification);
}
