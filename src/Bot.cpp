#include "../includes/Bot.hpp"

Bot::Bot(const std::string& nickname, const std::string& username, const std::string& realname, const std::string& channel)
    : _nickname(nickname), _username(username), _realname(realname), _channel(channel) {
    std::srand(static_cast<unsigned int>(std::time(NULL)));
    initResponses();
}

Bot::~Bot() {}

void Bot::initResponses() {
    _responses.push_back("Bonjour! Comment puis-je vous aider?");
    _responses.push_back("Intéressant, dites m'en plus.");
    _responses.push_back("Je suis un bot IRC simple.");
    _responses.push_back("N'hésitez pas à poser des questions!");
    _responses.push_back("Je ne comprends pas tout, mais j'essaie de faire de mon mieux.");
    _responses.push_back("C'est une belle journée pour discuter, n'est-ce pas?");
    _responses.push_back("J'adore les conversations IRC!");
    _responses.push_back("Avez-vous essayé de redémarrer votre ordinateur?");
    _responses.push_back("42 est la réponse à la grande question sur la vie, l'univers et le reste.");
    _responses.push_back("Parfois, le silence est la meilleure réponse.");
}

std::string Bot::getRandomResponse() const {
    if (_responses.empty())
        return "Je n'ai rien à dire.";

    size_t index = std::rand() % _responses.size();
    return _responses[index];
}

const std::string& Bot::getNickname() const {
    return _nickname;
}

const std::string& Bot::getUsername() const {
    return _username;
}

const std::string& Bot::getRealname() const {
    return _realname;
}

const std::string& Bot::getChannel() const {
    return _channel;
}
