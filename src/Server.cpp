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

// Structure pour représenter un utilisateur
struct User {
    std::string nickname;
    std::string username;
    bool authenticated;

    User() : authenticated(false) {}
};

// Structure pour représenter un canal
struct Channel {
    std::string name;
    std::set<int> members; // FDs des clients qui sont membres du canal
};

Server::Server(int port, const std::string& password)
    : port(port), password(password), server_fd(-1), epoll_fd(-1) {}

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

                std::cout << "New connection accepted!" << std::endl;

                // Send a welcome message to the client
                const char* welcome = "220 Welcome to the IRC server!\r\n";
                send(client_fd, welcome, strlen(welcome), 0);

                // Initialiser un nouvel utilisateur
                users[client_fd] = User();
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
                        std::cout << "Client disconnected" << std::endl;
                    else
                        std::cerr << "Error receiving data: " << strerror(errno) << std::endl;

                    // Retirer l'utilisateur de tous les canaux
                    std::map<std::string, Channel>::iterator channel_it;
                    for (channel_it = channels.begin(); channel_it != channels.end(); ++channel_it) {
                        channel_it->second.members.erase(client_fd);
                    }

                    // Supprimer l'utilisateur
                    users.erase(client_fd);

                    close(client_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    continue;
                }

                std::string msg(buffer);
                client_buffers[client_fd] += msg;

                // Traiter les commandes NICK et USER
                if (msg.find("NICK") == 0) {
                    std::string nickname = msg.substr(5);
                    size_t newline = nickname.find_first_of("\r\n");
                    if (newline != std::string::npos)
                        nickname = nickname.substr(0, newline);

                    users[client_fd].nickname = nickname;
                    std::string response = ":" + nickname + " NICK :" + nickname + "\r\n";
                    send(client_fd, response.c_str(), response.length(), 0);
                }
                else if (msg.find("USER") == 0) {
                    std::istringstream iss(msg.substr(5));
                    std::string username;
                    iss >> username;

                    users[client_fd].username = username;
                    users[client_fd].authenticated = true;

                    std::string response = ":ircserv 001 " + users[client_fd].nickname + " :Welcome to the IRC server!\r\n";
                    send(client_fd, response.c_str(), response.length(), 0);
                }
                else if (msg.find("JOIN") == 0)
                {
                    std::string channel_name = msg.substr(5);
                    size_t newline = channel_name.find_first_of("\r\n");
                    if (newline != std::string::npos)
                        channel_name = channel_name.substr(0, newline);

                    if (channel_name[0] != '#')
                        channel_name = "#" + channel_name;

                    // Vérifier si le canal existe, sinon le créer
                    if (channels.find(channel_name) == channels.end()) {
                        Channel new_channel;
                        new_channel.name = channel_name;
                        channels[channel_name] = new_channel;
                    }

                    // Ajouter l'utilisateur au canal
                    channels[channel_name].members.insert(client_fd);

                    std::string nick = users[client_fd].nickname;
                    if (nick.empty()) {
                        std::stringstream ss;
                        ss << "user" << client_fd;
                        nick = ss.str();
                    }

                    // Informer tous les membres du canal qu'un nouvel utilisateur a rejoint
                    std::string join_notification = ":" + nick + "!~" + users[client_fd].username + "@localhost JOIN :" + channel_name + "\r\n";

                    std::set<int>::iterator member_it;
                    for (member_it = channels[channel_name].members.begin();
                         member_it != channels[channel_name].members.end();
                         ++member_it) {
                        send(*member_it, join_notification.c_str(), join_notification.length(), 0);
                    }

                    // Construire la liste des membres pour la commande NAMES
                    std::string members_list;
                    for (member_it = channels[channel_name].members.begin();
                         member_it != channels[channel_name].members.end();
                         ++member_it) {
                        std::string member_nick = users[*member_it].nickname;
                        if (member_nick.empty()) {
                            std::stringstream ss;
                            ss << "user" << *member_it;
                            member_nick = ss.str();
                        }
                        members_list += member_nick + " ";
                    }

                    // Envoyer les réponses uniquement au client qui a rejoint
                    std::string topic_reply = ":ircserv 332 " + nick + " " + channel_name + " :Welcome to the channel!\r\n";
                    std::string names_reply = ":ircserv 353 " + nick + " = " + channel_name + " :" + members_list + "\r\n";
                    std::string end_names_reply = ":ircserv 366 " + nick + " " + channel_name + " :End of /NAMES list.\r\n";

                    send(client_fd, topic_reply.c_str(), topic_reply.length(), 0);
                    send(client_fd, names_reply.c_str(), names_reply.length(), 0);
                    send(client_fd, end_names_reply.c_str(), end_names_reply.length(), 0);
                }
                else if (msg.find("PRIVMSG") == 0)
                {
                    size_t space_pos = msg.find(' ', 8);
                    if (space_pos != std::string::npos) {
                        std::string target = msg.substr(8, space_pos - 8);
                        std::string message = msg.substr(space_pos + 1);

                        // Enlever le ":" au début du message et les CR/LF à la fin
                        if (message.size() > 0 && message[0] == ':') {
                            message = message.substr(1);
                        }
                        size_t crlf = message.find_first_of("\r\n");
                        if (crlf != std::string::npos) {
                            message = message.substr(0, crlf);
                        }

                        std::string sender = users[client_fd].nickname;
                        if (sender.empty()) {
                            std::stringstream ss;
                            ss << "user" << client_fd;
                            sender = ss.str();
                        }

                        std::string msg_notification = ":" + sender + "!~" + users[client_fd].username + "@localhost PRIVMSG " + target + " :" + message + "\r\n";

                        // Si c'est un message de canal
                        if (target[0] == '#') {
                            if (channels.find(target) != channels.end()) {
                                std::set<int>::iterator member_it;
                                for (member_it = channels[target].members.begin();
                                     member_it != channels[target].members.end();
                                     ++member_it) {
                                    if (*member_it != client_fd) { // Ne pas renvoyer le message à l'expéditeur
                                        send(*member_it, msg_notification.c_str(), msg_notification.length(), 0);
                                    }
                                }
                            }
                        }
                        // Sinon c'est un message privé entre utilisateurs
                    }
                }
                else
                {
                    std::string response = "Server echo: ";
                    response += buffer;
                    send(client_fd, response.c_str(), response.length(), 0);
                }
            }
        }
    }

    // Clean up
    close(server_fd);
    close(epoll_fd);
}
