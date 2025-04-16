#include <Command.hpp>
#include <sys/socket.h>

void Command::handleNick(int client_fd, std::istringstream& iss)
{
    std::string nickname;
    if (std::getline(iss, nickname) && !nickname.empty())
	{
        if (nickname[0] == ' ') nickname = nickname.substr(1);

        size_t crlf = nickname.find_first_of("\r\n");
        if (crlf != std::string::npos)
            nickname = nickname.substr(0, crlf);

        if (nickname.empty() || nickname.find(' ') != std::string::npos)
		{
            std::string error = ":ircserv 432 * :Erroneous nickname\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
        }
		else
		{
            bool nickname_in_use = false;
            for (std::map<int, User>::iterator it = users.begin(); it != users.end(); ++it) {
                if (it->first != client_fd && it->second.getNickname() == nickname)
				{
                    nickname_in_use = true;
                    break;
                }
            }

            if (nickname_in_use)
			{
                std::string error = ":ircserv 433 * " + nickname + " :Nickname is already in use\r\n";
                send(client_fd, error.c_str(), error.length(), 0);
            }
			else
			{
                std::string old_nick = users[client_fd].getNickname();
                users[client_fd].setNickname(nickname);

                std::string response;
                if (old_nick.empty())
                    response = ":" + nickname + " NICK :" + nickname + "\r\n";
				else
                    response = ":" + old_nick + "!~" + users[client_fd].getUsername() + "@localhost NICK :" + nickname + "\r\n";
                send(client_fd, response.c_str(), response.length(), 0);

                if (!users[client_fd].getUsername().empty() &&
                    users[client_fd].isPasswordVerified() &&
                    !users[client_fd].isAuthenticated())
				{
                    users[client_fd].setAuthenticated(true);
                    sendWelcomeMessages(client_fd, users[client_fd]);
                }
            }
        }
    }
}
