#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Channel {
private:
    std::string name;
    std::string topic;
    std::set<int> members;  // FDs des clients qui sont membres du canal
    std::set<int> operators; // FDs des clients qui sont opérateurs du canal

public:
    Channel();
    explicit Channel(const std::string& channelName);
    ~Channel();

    // Getters
    const std::string& getName() const;
    const std::string& getTopic() const;
    const std::set<int>& getMembers() const;
    const std::set<int>& getOperators() const;

    // Setters
    void setName(const std::string& channelName);
    void setTopic(const std::string& channelTopic);

    // Gestion des membres
    bool addMember(int client_fd);
    bool removeMember(int client_fd);
    bool hasMember(int client_fd) const;
    bool isEmpty() const;

    // Gestion des opérateurs
    bool addOperator(int client_fd);
    bool removeOperator(int client_fd);
    bool isOperator(int client_fd) const;

    // Utilitaires
    void broadcastMessage(const std::string& message, int excludeClient = -1) const;
};

#endif
