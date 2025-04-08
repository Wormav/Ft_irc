#ifndef USER_HPP
#define USER_HPP

#include <string>

class User {
private:
    std::string nickname;
    std::string username;
    std::string realname;
    bool authenticated;
    bool password_verified;

public:
    User();
    ~User();

    // Getters
    const std::string& getNickname() const;
    const std::string& getUsername() const;
    const std::string& getRealname() const;
    bool isAuthenticated() const;
    bool isPasswordVerified() const;

    // Setters
    void setNickname(const std::string& nick);
    void setUsername(const std::string& user);
    void setRealname(const std::string& real);
    void setAuthenticated(bool auth);
    void setPasswordVerified(bool verified);

    // Utilitaires
    std::string getFullIdentity() const;  // Returns nickname!~username@localhost
};

#endif