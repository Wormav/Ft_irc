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
    std::set<int> invited;   // FDs des clients qui sont invités au canal

    // Modes de canal
    bool inviteOnly;       // Mode +i
    bool topicRestricted;  // Mode +t
    bool hasUserLimit;     // Mode +l
    bool hasKey;           // Mode +k
    std::string key;       // Clé pour le mode +k
    size_t userLimit;      // Limite pour le mode +l

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

    // Gestion des invitations
    bool addInvite(int client_fd);
    bool removeInvite(int client_fd);
    bool isInvited(int client_fd) const;

    // Gestion des modes de canal
    bool isInviteOnly() const;
    bool isTopicRestricted() const;
    bool hasKeySet() const;
    bool hasUserLimitSet() const;
    const std::string& getKey() const;
    size_t getUserLimit() const;

    void setInviteOnly(bool value);
    void setTopicRestricted(bool value);
    void setKey(const std::string& newKey);
    void removeKey();
    void setUserLimit(size_t limit);
    void removeUserLimit();

    // Obtenir la chaîne des modes actifs
    std::string getModeString() const;

    // Utilitaires
    void broadcastMessage(const std::string& message, int excludeClient = -1) const;
};

#endif
