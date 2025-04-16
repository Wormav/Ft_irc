#include <Command.hpp>
#include <sys/socket.h>

void Command::handleInvite(int client_fd, const std::string& line)
{
    if (!users[client_fd].isAuthenticated())
	{
        std::string error = ":ircserv 451 * :You have not registered\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    std::istringstream iss;
    if (line.length() > 7)
        iss.str(line.substr(7));
    std::string nickname, channel_name;

    iss >> nickname >> channel_name;

    if (nickname.empty() || channel_name.empty())
	{
        std::string error = ":ircserv 461 " + users[client_fd].getNickname() + " INVITE :Not enough parameters\r\n";
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

    if (!channel_it->second.isOperator(client_fd))
	{
        std::string error = ":ircserv 482 " + users[client_fd].getNickname() + " " + channel_name + " :You're not channel operator\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    int target_fd = -1;
    for (std::map<int, User>::iterator it = users.begin(); it != users.end(); ++it) {
        if (it->second.getNickname() == nickname)
		{
            target_fd = it->first;
            break;
        }
    }

    if (target_fd == -1)
	{
        std::string error = ":ircserv 401 " + users[client_fd].getNickname() + " " + nickname + " :No such nick/channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    if (channel_it->second.hasMember(target_fd))
	{
        std::string error = ":ircserv 443 " + users[client_fd].getNickname() + " " + nickname + " " + channel_name + " :is already on channel\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
        return;
    }

    channel_it->second.addInvite(target_fd);

    std::string invite_notification = ":" + users[client_fd].getFullIdentity() + " INVITE " + nickname + " :" + channel_name + "\r\n";
    send(target_fd, invite_notification.c_str(), invite_notification.length(), 0);

    std::string invite_confirm = ":ircserv 341 " + users[client_fd].getNickname() + " " + nickname + " " + channel_name + "\r\n";
    send(client_fd, invite_confirm.c_str(), invite_confirm.length(), 0);
}
