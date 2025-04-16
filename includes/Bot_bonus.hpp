#ifndef BOT_BONUS_HPP
# define BOT_BONUS_HPP

#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>

#define BUFFER_SIZE 1024

struct BotParams
{
    int port;
    std::string password;
};

class Bot
{
	private:
		std::string         _nickname;
		std::string         _username;
		std::string         _realname;
		std::string         _channel;
		std::vector<std::string> _responses;

	public:
		Bot(const std::string& nickname, const std::string& username, const std::string& realname, const std::string& channel);
		~Bot();
		void initResponses();

		std::string getRandomResponse() const;

		const std::string& getNickname() const;
		const std::string& getUsername() const;
		const std::string& getRealname() const;
		const std::string& getChannel() const;
};

#endif
