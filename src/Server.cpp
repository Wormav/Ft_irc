#include <Server.hpp>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <sys/epoll.h>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>

// Structure pour représenter un utilisateur
struct User {
    std::string nickname;
    std::string username;
    std::string realname;
    bool authenticated;
    bool password_verified;

    User() : authenticated(false), password_verified(false) {}
};

// Structure pour représenter un canal
struct Channel {
    std::string name;
    std::string topic;
    std::set<int> members; // FDs des clients qui sont membres du canal
};

Server::Server(int port, const std::string& password)
    : port(port), password(password), server_fd(-1), epoll_fd(-1) {}

// Déclaration anticipée de la fonction sendWelcomeMessages
void sendWelcomeMessages(int client_fd, const User& user);

// Traiter une ligne de commande reçue d'un client
void processCommand(int client_fd, const std::string& line, 
                   std::map<int, User>& users,
                   std::map<std::string, Channel>& channels,
                   const std::string& server_password) {
    
    std::istringstream iss(line);
    std::string command;
    iss >> command;

    // Convertir la commande en majuscules pour une comparaison insensible à la casse
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);

    if (command == "PASS") {
        std::string pass;
        if (std::getline(iss, pass) && !pass.empty()) {
            // Supprimer l'espace initial et nettoyer la chaîne
            if (pass[0] == ' ') pass = pass.substr(1);
            
            // Supprimer les caractères CR/LF s'ils existent
            size_t crlf = pass.find_first_of("\r\n");
            if (crlf != std::string::npos) {
                pass = pass.substr(0, crlf);
            }

            if (pass == server_password) {
                users[client_fd].password_verified = true;
            } else {
                std::string error = ":ircserv 464 * :Password incorrect\r\n";
                send(client_fd, error.c_str(), error.length(), 0);
            }
        }
    }
    else if (command == "NICK") {
        std::string nickname;
        if (std::getline(iss, nickname) && !nickname.empty()) {
            // Supprimer l'espace initial et nettoyer la chaîne
            if (nickname[0] == ' ') nickname = nickname.substr(1);
            
            size_t crlf = nickname.find_first_of("\r\n");
            if (crlf != std::string::npos) {
                nickname = nickname.substr(0, crlf);
            }
            
            // Vérifier si le surnom est vide ou contient des caractères invalides
            if (nickname.empty() || nickname.find(' ') != std::string::npos) {
                std::string error = ":ircserv 432 * :Erroneous nickname\r\n";
                send(client_fd, error.c_str(), error.length(), 0);
            } else {
                // Vérifier si le surnom est déjà utilisé
                bool nickname_in_use = false;
                for (std::map<int, User>::iterator it = users.begin(); it != users.end(); ++it) {
                    if (it->first != client_fd && it->second.nickname == nickname) {
                        nickname_in_use = true;
                        break;
                    }
                }
                
                if (nickname_in_use) {
                    std::string error = ":ircserv 433 * " + nickname + " :Nickname is already in use\r\n";
                    send(client_fd, error.c_str(), error.length(), 0);
                } else {
                    std::string old_nick = users[client_fd].nickname;
                    users[client_fd].nickname = nickname;
                    
                    std::string response;
                    if (old_nick.empty()) {
                        response = ":" + nickname + " NICK :" + nickname + "\r\n";
                    } else {
                        response = ":" + old_nick + "!~" + users[client_fd].username + "@localhost NICK :" + nickname + "\r\n";
                    }
                    send(client_fd, response.c_str(), response.length(), 0);
                    
                    // Si l'utilisateur a déjà envoyé USER et PASS, compléter l'authentification
                    if (!users[client_fd].username.empty() && users[client_fd].password_verified && !users[client_fd].authenticated) {
                        users[client_fd].authenticated = true;
                        sendWelcomeMessages(client_fd, users[client_fd]);
                    }
                }
            }
        }
    }
    else if (command == "USER") {
        if (!users[client_fd].password_verified) {
            std::string error = ":ircserv 464 * :Password required before registration\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
            return;
        }
        
        std::string params = line.substr(5); // Skip "USER "
        std::istringstream params_iss(params);
        std::string username, mode, unused;
        params_iss >> username >> mode >> unused;
        
        // Le realname est tout ce qui suit le dernier paramètre (peut contenir des espaces)
        std::string realname;
        if (std::getline(params_iss, realname) && !realname.empty()) {
            if (realname[0] == ' ') realname = realname.substr(1);
            if (realname[0] == ':') realname = realname.substr(1);
        }
        
        users[client_fd].username = username;
        users[client_fd].realname = realname;
        
        // Si l'utilisateur a déjà envoyé NICK, compléter l'authentification
        if (!users[client_fd].nickname.empty() && !users[client_fd].authenticated) {
            users[client_fd].authenticated = true;
            sendWelcomeMessages(client_fd, users[client_fd]);
        }
    }
    else if (command == "JOIN") {
        // Vérifier l'authentification
        if (!users[client_fd].authenticated) {
            std::string error = ":ircserv 451 * :You have not registered\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
            return;
        }
        
        std::string channel_params;
        std::getline(iss, channel_params);
        if (channel_params[0] == ' ') channel_params = channel_params.substr(1);
        
        std::istringstream channel_iss(channel_params);
        std::string channel_list;
        channel_iss >> channel_list;
        
        // Traiter chaque canal dans la liste (séparés par des virgules)
        std::istringstream channel_tokens(channel_list);
        std::string channel_name;
        while (std::getline(channel_tokens, channel_name, ',')) {
            if (channel_name.empty()) continue;
            
            // Ajouter # si nécessaire
            if (channel_name[0] != '#') {
                channel_name = "#" + channel_name;
            }
            
            // Vérifier si le canal existe, sinon le créer
            if (channels.find(channel_name) == channels.end()) {
                Channel new_channel;
                new_channel.name = channel_name;
                new_channel.topic = "Welcome to " + channel_name;
                channels[channel_name] = new_channel;
            }
            
            // Ajouter l'utilisateur au canal
            channels[channel_name].members.insert(client_fd);
            
            std::string nick = users[client_fd].nickname;
            
            // Informer tous les membres du canal qu'un nouvel utilisateur a rejoint
            std::string join_notification = ":" + nick + "!~" + users[client_fd].username + "@localhost JOIN :" + channel_name + "\r\n";
            
            for (std::set<int>::iterator member_it = channels[channel_name].members.begin();
                 member_it != channels[channel_name].members.end();
                 ++member_it) {
                send(*member_it, join_notification.c_str(), join_notification.length(), 0);
            }
            
            // Envoyer les informations du canal à l'utilisateur qui vient de rejoindre
            if (!channels[channel_name].topic.empty()) {
                std::string topic_reply = ":ircserv 332 " + nick + " " + channel_name + " :" + channels[channel_name].topic + "\r\n";
                send(client_fd, topic_reply.c_str(), topic_reply.length(), 0);
            }
            
            // Construire la liste des membres pour NAMES
            std::string members_list;
            for (std::set<int>::iterator member_it = channels[channel_name].members.begin();
                 member_it != channels[channel_name].members.end();
                 ++member_it) {
                std::string member_nick = users[*member_it].nickname;
                members_list += member_nick + " ";
            }
            
            std::string names_reply = ":ircserv 353 " + nick + " = " + channel_name + " :" + members_list + "\r\n";
            std::string end_names_reply = ":ircserv 366 " + nick + " " + channel_name + " :End of /NAMES list.\r\n";
            
            send(client_fd, names_reply.c_str(), names_reply.length(), 0);
            send(client_fd, end_names_reply.c_str(), end_names_reply.length(), 0);
        }
    }
    else if (command == "PRIVMSG" || command == "NOTICE") {
        // Vérifier l'authentification
        if (!users[client_fd].authenticated) {
            if (command == "PRIVMSG") { // NOTICE ne doit pas générer de réponse d'erreur
                std::string error = ":ircserv 451 * :You have not registered\r\n";
                send(client_fd, error.c_str(), error.length(), 0);
            }
            return;
        }
        
        std::string target;
        iss >> target;
        
        std::string message;
        std::getline(iss, message);
        if (message.empty()) return;
        
        // Supprimer l'espace initial
        if (message[0] == ' ') message = message.substr(1);
        
        // Supprimer le ":" au début du message si présent
        if (message[0] == ':') message = message.substr(1);
        
        std::string sender = users[client_fd].nickname;
        std::string msg_notification = ":" + sender + "!~" + users[client_fd].username + "@localhost " + command + " " + target + " :" + message + "\r\n";
        
        // Si c'est un message de canal
        if (target[0] == '#') {
            std::map<std::string, Channel>::iterator channel_it = channels.find(target);
            if (channel_it != channels.end()) {
                // Vérifier si l'utilisateur est dans le canal
                if (channel_it->second.members.find(client_fd) == channel_it->second.members.end()) {
                    if (command == "PRIVMSG") { // NOTICE ne doit pas générer de réponse d'erreur
                        std::string error = ":ircserv 442 " + sender + " " + target + " :You're not on that channel\r\n";
                        send(client_fd, error.c_str(), error.length(), 0);
                    }
                    return;
                }
                
                // Envoyer le message à tous les membres du canal sauf l'expéditeur
                for (std::set<int>::iterator member_it = channel_it->second.members.begin();
                     member_it != channel_it->second.members.end();
                     ++member_it) {
                    if (*member_it != client_fd) {
                        send(*member_it, msg_notification.c_str(), msg_notification.length(), 0);
                    }
                }
            } else {
                if (command == "PRIVMSG") { // NOTICE ne doit pas générer de réponse d'erreur
                    std::string error = ":ircserv 403 " + sender + " " + target + " :No such channel\r\n";
                    send(client_fd, error.c_str(), error.length(), 0);
                }
            }
        }
        // Sinon c'est un message privé entre utilisateurs
        else {
            bool user_found = false;
            for (std::map<int, User>::iterator it = users.begin(); it != users.end(); ++it) {
                if (it->second.nickname == target) {
                    user_found = true;
                    send(it->first, msg_notification.c_str(), msg_notification.length(), 0);
                    break;
                }
            }
            
            if (!user_found && command == "PRIVMSG") { // NOTICE ne doit pas générer de réponse d'erreur
                std::string error = ":ircserv 401 " + sender + " " + target + " :No such nick/channel\r\n";
                send(client_fd, error.c_str(), error.length(), 0);
            }
        }
    }
    else if (command == "PING") {
        std::string token;
        iss >> token;
        std::string response = "PONG " + token + "\r\n";
        send(client_fd, response.c_str(), response.length(), 0);
    }
    else if (command == "PART") {
        // Vérifier l'authentification
        if (!users[client_fd].authenticated) {
            std::string error = ":ircserv 451 * :You have not registered\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
            return;
        }
        
        std::string channel_params;
        std::getline(iss, channel_params);
        if (channel_params[0] == ' ') channel_params = channel_params.substr(1);
        
        std::istringstream channel_iss(channel_params);
        std::string channel_list;
        channel_iss >> channel_list;
        
        std::string part_reason;
        std::getline(channel_iss, part_reason);
        if (!part_reason.empty() && part_reason[0] == ' ') part_reason = part_reason.substr(1);
        if (!part_reason.empty() && part_reason[0] == ':') part_reason = part_reason.substr(1);
        if (part_reason.empty()) part_reason = "Leaving";
        
        // Traiter chaque canal dans la liste
        std::istringstream channel_tokens(channel_list);
        std::string channel_name;
        while (std::getline(channel_tokens, channel_name, ',')) {
            std::map<std::string, Channel>::iterator channel_it = channels.find(channel_name);
            if (channel_it != channels.end()) {
                std::set<int>::iterator member_it = channel_it->second.members.find(client_fd);
                if (member_it != channel_it->second.members.end()) {
                    std::string nick = users[client_fd].nickname;
                    std::string part_notification = ":" + nick + "!~" + users[client_fd].username + "@localhost PART " + channel_name + " :" + part_reason + "\r\n";
                    
                    // Informer tous les membres du canal
                    for (std::set<int>::iterator it = channel_it->second.members.begin();
                         it != channel_it->second.members.end();
                         ++it) {
                        send(*it, part_notification.c_str(), part_notification.length(), 0);
                    }
                    
                    // Retirer l'utilisateur du canal
                    channel_it->second.members.erase(member_it);
                    
                    // Si le canal est vide, on peut le supprimer
                    if (channel_it->second.members.empty()) {
                        channels.erase(channel_it);
                    }
                } else {
                    std::string error = ":ircserv 442 " + users[client_fd].nickname + " " + channel_name + " :You're not on that channel\r\n";
                    send(client_fd, error.c_str(), error.length(), 0);
                }
            } else {
                std::string error = ":ircserv 403 " + users[client_fd].nickname + " " + channel_name + " :No such channel\r\n";
                send(client_fd, error.c_str(), error.length(), 0);
            }
        }
    }
    else if (command == "QUIT") {
        std::string quit_reason;
        std::getline(iss, quit_reason);
        if (!quit_reason.empty() && quit_reason[0] == ' ') quit_reason = quit_reason.substr(1);
        if (!quit_reason.empty() && quit_reason[0] == ':') quit_reason = quit_reason.substr(1);
        if (quit_reason.empty()) quit_reason = "Quit";
        
        // Informer tous les canaux où l'utilisateur est membre
        std::string nick = users[client_fd].nickname;
        std::string quit_notification = ":" + nick + "!~" + users[client_fd].username + "@localhost QUIT :Quit: " + quit_reason + "\r\n";
        
        std::set<int> informed_users;
        
        for (std::map<std::string, Channel>::iterator channel_it = channels.begin();
             channel_it != channels.end();
             ++channel_it) {
            std::set<int>::iterator member_it = channel_it->second.members.find(client_fd);
            if (member_it != channel_it->second.members.end()) {
                // Informer tous les membres du canal
                for (std::set<int>::iterator it = channel_it->second.members.begin();
                     it != channel_it->second.members.end();
                     ++it) {
                    if (*it != client_fd && informed_users.find(*it) == informed_users.end()) {
                        send(*it, quit_notification.c_str(), quit_notification.length(), 0);
                        informed_users.insert(*it);
                    }
                }
                
                // Retirer l'utilisateur du canal
                channel_it->second.members.erase(member_it);
            }
        }
        
        // Nettoyer les canaux vides
        std::map<std::string, Channel>::iterator channel_it = channels.begin();
        while (channel_it != channels.end()) {
            if (channel_it->second.members.empty()) {
                std::map<std::string, Channel>::iterator to_erase = channel_it;
                ++channel_it;
                channels.erase(to_erase);
            } else {
                ++channel_it;
            }
        }
        
        // Fermer la connexion client
        close(client_fd);
    }
    else {
        // Commande non reconnue ou non implémentée
        std::string error = ":ircserv 421 " + (users[client_fd].authenticated ? users[client_fd].nickname : std::string("*")) + " " + command + " :Unknown command\r\n";
        send(client_fd, error.c_str(), error.length(), 0);
    }
}

// Envoie les messages de bienvenue à un client nouvellement authentifié
void sendWelcomeMessages(int client_fd, const User& user) {
    std::string welcome = ":ircserv 001 " + user.nickname + " :Welcome to the IRC Network " + user.nickname + "!~" + user.username + "@localhost\r\n";
    std::string yourhost = ":ircserv 002 " + user.nickname + " :Your host is ircserv, running version 1.0\r\n";
    std::string created = ":ircserv 003 " + user.nickname + " :This server was created Apr 2025\r\n";
    std::string myinfo = ":ircserv 004 " + user.nickname + " ircserv 1.0 o o\r\n";
    
    send(client_fd, welcome.c_str(), welcome.length(), 0);
    send(client_fd, yourhost.c_str(), yourhost.length(), 0);
    send(client_fd, created.c_str(), created.length(), 0);
    send(client_fd, myinfo.c_str(), myinfo.length(), 0);
}

void Server::run()
{
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return;
    }

    // Configuring the socket to reuse the address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "Error configuring socket: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    // Server Address Configuration
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Accepts connections from any interface
    address.sin_port = htons(port);

    // Binding socket to address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Error during bind: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    // Socket listening (with a queue of 10 connections)
    if (listen(server_fd, 10) < 0)
    {
        std::cerr << "Error while listening: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    // Create epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
    {
        std::cerr << "Error creating epoll instance: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    // Add server socket to epoll
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0)
    {
        std::cerr << "Error adding server socket to epoll: " << strerror(errno) << std::endl;
        close(server_fd);
        close(epoll_fd);
        return;
    }

    std::cout << "Server started on port " << port << std::endl;
    std::cout << "Waiting for connections..." << std::endl;

    // Main epoll loop
    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];
    std::map<int, std::string> client_buffers;
    std::map<int, User> users;
    std::map<std::string, Channel> channels;

    while (true)
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds < 0)
        {
            std::cerr << "Error during epoll_wait: " << strerror(errno) << std::endl;
            break;
        }

        for (int i = 0; i < nfds; ++i)
        {
            if (events[i].data.fd == server_fd)
            {
                // New connection
                int client_fd;
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);

                client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
                if (client_fd < 0)
                {
                    std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
                    continue;
                }

                // Add client socket to epoll
                event.events = EPOLLIN;
                event.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0)
                {
                    std::cerr << "Error adding client socket to epoll: " << strerror(errno) << std::endl;
                    close(client_fd);
                    continue;
                }

                std::cout << "New connection accepted! Client fd: " << client_fd << std::endl;

                // Initialiser un nouvel utilisateur et buffer
                users[client_fd] = User();
                client_buffers[client_fd] = "";
            }
            else
            {
                // Data from client
                int client_fd = events[i].data.fd;
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

                if (bytes_received <= 0)
                {
                    // Client disconnected or error
                    if (bytes_received == 0)
                        std::cout << "Client disconnected (fd: " << client_fd << ")" << std::endl;
                    else
                        std::cerr << "Error receiving data: " << strerror(errno) << std::endl;

                    // Informer les autres utilisateurs du départ
                    if (users.find(client_fd) != users.end() && !users[client_fd].nickname.empty()) {
                        std::string nick = users[client_fd].nickname;
                        std::string quit_notification = ":" + nick + "!~" + users[client_fd].username + "@localhost QUIT :Connection closed\r\n";
                        
                        std::set<int> informed_users;
                        
                        for (std::map<std::string, Channel>::iterator channel_it = channels.begin();
                             channel_it != channels.end();
                             ++channel_it) {
                            std::set<int>::iterator member_it = channel_it->second.members.find(client_fd);
                            if (member_it != channel_it->second.members.end()) {
                                // Informer tous les membres du canal
                                for (std::set<int>::iterator it = channel_it->second.members.begin();
                                     it != channel_it->second.members.end();
                                     ++it) {
                                    if (*it != client_fd && informed_users.find(*it) == informed_users.end()) {
                                        send(*it, quit_notification.c_str(), quit_notification.length(), 0);
                                        informed_users.insert(*it);
                                    }
                                }
                                
                                // Retirer l'utilisateur du canal
                                channel_it->second.members.erase(member_it);
                            }
                        }
                    }

                    // Nettoyer les canaux vides
                    std::map<std::string, Channel>::iterator channel_it = channels.begin();
                    while (channel_it != channels.end()) {
                        if (channel_it->second.members.empty()) {
                            std::map<std::string, Channel>::iterator to_erase = channel_it;
                            ++channel_it;
                            channels.erase(to_erase);
                        } else {
                            ++channel_it;
                        }
                    }

                    // Nettoyer les ressources du client
                    client_buffers.erase(client_fd);
                    users.erase(client_fd);
                    close(client_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    continue;
                }

                // Ajouter les données au buffer du client
                client_buffers[client_fd] += buffer;

                // Traiter les commandes complètes (se terminant par \r\n)
                std::string& buf = client_buffers[client_fd];
                size_t pos;
                while ((pos = buf.find("\r\n")) != std::string::npos) {
                    std::string line = buf.substr(0, pos);
                    buf = buf.substr(pos + 2); // Supprimer la ligne traitée du buffer
                    
                    std::cout << "Received from client " << client_fd << ": " << line << std::endl;
                    
                    // Traiter la commande
                    processCommand(client_fd, line, users, channels, password);
                }
            }
        }
    }

    // Clean up
    close(server_fd);
    close(epoll_fd);
}