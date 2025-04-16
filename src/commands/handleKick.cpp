#include <Command.hpp>
#include <sys/socket.h>

void Command::handleKick(int client_fd, const std::string& line)
{
    if (!users[client_fd].isAuthenticated())
	{
        std::string error = ":ircserv 451 * :You have not registered\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    std::istringstream iss;
    if (line.length() > 5)
        iss.str(line.substr(5));

    std::string channel_name, target_nick;

    iss >> channel_name >> target_nick;

    if (channel_name.empty() || target_nick.empty())
	{
        std::string error = ":ircserv 461 " + users[client_fd].getNickname() + " KICK :Not enough parameters\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    if (channel_name[0] != '#')
        channel_name = "#" + channel_name;

    std::string kick_message = users[client_fd].getNickname();
    std::string rest;
    std::getline(iss, rest);

    size_t colon_pos = rest.find(':');
    if (colon_pos != std::string::npos)
        kick_message = rest.substr(colon_pos + 1);

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

    if (!channel_it->second.isOperator(client_fd))
	{
        std::string error = ":ircserv 482 " + users[client_fd].getNickname() + " " + channel_name + " :You're not channel operator\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    int target_fd = -1;
    for (std::map<int, User>::iterator it = users.begin(); it != users.end(); ++it)
	{
        if (it->second.getNickname() == target_nick)
		{
            target_fd = it->first;
            break;
        }
    }

    if (target_fd == -1 || !channel_it->second.hasMember(target_fd))
	{
        std::string error = ":ircserv 441 " + users[client_fd].getNickname() + " " + target_nick + " " + channel_name + " :They aren't on that channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    std::string kick_notification = ":" + users[client_fd].getFullIdentity() + " KICK " + channel_name + " " + target_nick + " :" + kick_message + "\r\n";
    channel_it->second.broadcastMessage(kick_notification);

    channel_it->second.removeMember(target_fd);
}
