#include <Bot_bonus.hpp>

Bot::Bot(const std::string& nickname, const std::string& username, const std::string& realname, const std::string& channel)
    : _nickname(nickname), _username(username), _realname(realname), _channel(channel)
{
    srand(time(NULL));
    initResponses();
}

Bot::~Bot() {}

void Bot::initResponses()
{
    _responses.push_back("Bonjour ! Comment puis-je vous aider ?");
    _responses.push_back("Je suis un bot IRC simple.");
    _responses.push_back("Je ne comprends pas encore beaucoup de commandes.");
    _responses.push_back("C'est une belle journée pour discuter sur IRC !");
    _responses.push_back("N'hésitez pas à poser des questions.");
    _responses.push_back("42 est la réponse à la grande question sur la vie, l'univers et le reste.");
}

std::string Bot::getRandomResponse() const
{
    if (_responses.empty())
        return "Je n'ai rien à dire.";

    return _responses[rand() % _responses.size()];
}

const std::string& Bot::getNickname() const
{
    return _nickname;
}

const std::string& Bot::getUsername() const
{
    return _username;
}

const std::string& Bot::getRealname() const
{
    return _realname;
}

const std::string& Bot::getChannel() const
{
    return _channel;
}
