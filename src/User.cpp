#include "User.hpp"

User::User() : authenticated(false), password_verified(false) {}

User::~User() {}

// Getters
const std::string& User::getNickname() const {
    return nickname;
}

const std::string& User::getUsername() const {
    return username;
}

const std::string& User::getRealname() const {
    return realname;
}

bool User::isAuthenticated() const {
    return authenticated;
}

bool User::isPasswordVerified() const {
    return password_verified;
}

// Setters
void User::setNickname(const std::string& nick) {
    nickname = nick;
}

void User::setUsername(const std::string& user) {
    username = user;
}

void User::setRealname(const std::string& real) {
    realname = real;
}

void User::setAuthenticated(bool auth) {
    authenticated = auth;
}

void User::setPasswordVerified(bool verified) {
    password_verified = verified;
}

// Utilitaires
std::string User::getFullIdentity() const {
    return nickname + "!~" + username + "@localhost";
}