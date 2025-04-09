#include "Server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <sys/epoll.h>
#include <sstream>
#include <algorithm>

Server::Server(int port, const std::string& password)
    : port(port), password(password), server_fd(-1), epoll_fd(-1) {
    command_handler = new Command(this, users, channels, password);
}

Server::~Server() {
    if (server_fd >= 0) {
        close(server_fd);
    }

    if (epoll_fd >= 0) {
        close(epoll_fd);
    }

    delete command_handler;
}

void Server::setupSocket() {
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return;
    }

    // Configuring the socket to reuse the address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error configuring socket: " << strerror(errno) << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }

    // Server Address Configuration
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Accepts connections from any interface
    address.sin_port = htons(port);

    // Binding socket to address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Error during bind: " << strerror(errno) << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }

    // Socket listening (with a queue of 10 connections)
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Error while listening: " << strerror(errno) << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }

    // Create epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        std::cerr << "Error creating epoll instance: " << strerror(errno) << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }

    // Add server socket to epoll
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
        std::cerr << "Error adding server socket to epoll: " << strerror(errno) << std::endl;
        close(server_fd);
        close(epoll_fd);
        server_fd = -1;
        epoll_fd = -1;
        return;
    }

    std::cout << "Server started on port " << port << std::endl;
    std::cout << "Waiting for connections..." << std::endl;
}

void Server::handleNewConnection() {
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0) {
        std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
        return;
    }

    // Add client socket to epoll
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
        std::cerr << "Error adding client socket to epoll: " << strerror(errno) << std::endl;
        close(client_fd);
        return;
    }

    std::cout << "New connection accepted! Client fd: " << client_fd << std::endl;

    // Initialiser un nouvel utilisateur et buffer
    users[client_fd] = User();
    client_buffers[client_fd] = "";
}

void Server::handleClientData(int client_fd) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        // Client disconnected or error
        if (bytes_received == 0)
            std::cout << "Client disconnected (fd: " << client_fd << ")" << std::endl;
        else
            std::cerr << "Error receiving data: " << strerror(errno) << std::endl;

        disconnectClient(client_fd);
        return;
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
        processCommand(client_fd, line);
    }
}

void Server::disconnectClient(int client_fd) {
    // Informer les autres utilisateurs du départ
    if (users.find(client_fd) != users.end() && !users[client_fd].getNickname().empty()) {
        std::string quit_notification = ":" + users[client_fd].getFullIdentity() + " QUIT :Connection closed\r\n";

        std::set<int> informed_users;

        for (std::map<std::string, Channel>::iterator channel_it = channels.begin();
             channel_it != channels.end();
             ++channel_it) {
            if (channel_it->second.hasMember(client_fd)) {
                // Informer tous les membres du canal
                const std::set<int>& members = channel_it->second.getMembers();
                for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it) {
                    if (*it != client_fd && informed_users.find(*it) == informed_users.end()) {
                        send(*it, quit_notification.c_str(), quit_notification.length(), 0);
                        informed_users.insert(*it);
                    }
                }

                // Retirer l'utilisateur du canal
                channel_it->second.removeMember(client_fd);
            }
        }
    }

    // Nettoyer les canaux vides
    std::map<std::string, Channel>::iterator channel_it = channels.begin();
    while (channel_it != channels.end()) {
        if (channel_it->second.isEmpty()) {
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
}

void Server::processCommand(int client_fd, const std::string& line) {
    // Déléguer le traitement à la classe Command
    command_handler->process(client_fd, line);
}

void Server::run() {
    setupSocket();

    if (server_fd < 0 || epoll_fd < 0) {
        std::cerr << "Server setup failed. Exiting." << std::endl;
        return;
    }

    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];

    while (true) {
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if (n_events < 0) {
            std::cerr << "Error in epoll_wait: " << strerror(errno) << std::endl;
            break;
        }

        for (int i = 0; i < n_events; i++) {
            if (events[i].data.fd == server_fd) {
                // Nouvelle connexion
                handleNewConnection();
            } else {
                // Données reçues d'un client existant
                handleClientData(events[i].data.fd);
            }
        }
    }
}
