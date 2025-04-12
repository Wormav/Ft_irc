#ifndef BOT_HPP
# define BOT_HPP

#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>

class Bot {
private:
    std::string         _nickname;
    std::string         _username;
    std::string         _realname;
    std::string         _channel;
    std::vector<std::string> _responses;

public:
    Bot(const std::string& nickname, const std::string& username, const std::string& realname, const std::string& channel);
    ~Bot();

    // Initialiser les réponses possibles du bot
    void initResponses();

    // Obtenir une réponse aléatoire
    std::string getRandomResponse() const;

    // Getters
    const std::string& getNickname() const;
    const std::string& getUsername() const;
    const std::string& getRealname() const;
    const std::string& getChannel() const;
};

#endif
