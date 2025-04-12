#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include "../includes/Bot.hpp"

#define BUFFER_SIZE 1024

int connectToServer(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Erreur lors de la création du socket" << std::endl;
        return -1;
    }

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);

    // Utiliser localhost (127.0.0.1) par défaut
    if (inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr) <= 0) {
        std::cerr << "Adresse invalide" << std::endl;
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        std::cerr << "Connexion échouée" << std::endl;
        close(sockfd);
        return -1;
    }

    return sockfd;
}

void sendMessage(int sockfd, const std::string& message) {
    if (send(sockfd, message.c_str(), message.length(), 0) < 0) {
        std::cerr << "Erreur lors de l'envoi du message" << std::endl;
    }
}

void registerBot(int sockfd, const Bot& bot, const std::string& password) {
    sendMessage(sockfd, "PASS " + password + "\r\n");
    sendMessage(sockfd, "NICK " + bot.getNickname() + "\r\n");
    sendMessage(sockfd, "USER " + bot.getUsername() + " 0 * :" + bot.getRealname() + "\r\n");
}

void joinChannel(int sockfd, const std::string& channel) {
    sendMessage(sockfd, "JOIN " + channel + "\r\n");
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    std::string password = argv[2];

    Bot bot("IRCBot", "bot", "IRC Bot", "#bot");

    int sockfd = connectToServer(port);
    if (sockfd < 0) {
        return 1;
    }

    registerBot(sockfd, bot, password);

    // Attendre un peu pour s'assurer que l'enregistrement est terminé
    sleep(2);

    joinChannel(sockfd, bot.getChannel());

    char buffer[BUFFER_SIZE];
    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    std::cout << "Bot connecté au serveur IRC" << std::endl;

    while (true) {
        int ret = poll(fds, 1, -1);

        if (ret < 0) {
            std::cerr << "Erreur de poll" << std::endl;
            break;
        }

        if (fds[0].revents & POLLIN) {
            ssize_t bytesRead = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (bytesRead <= 0) {
                std::cerr << "Connexion fermée ou erreur" << std::endl;
                break;
            }

            buffer[bytesRead] = '\0';
            std::cout << "Reçu: " << buffer << std::endl;

            std::string message(buffer);

            // Répondre au PING pour rester connecté
            if (message.find("PING") != std::string::npos) {
                std::string pong = "PONG" + message.substr(4) + "\r\n";
                sendMessage(sockfd, pong);
            }

            // Répondre aux messages dans le canal
            if (message.find("PRIVMSG " + bot.getChannel()) != std::string::npos) {
                std::string response = "PRIVMSG " + bot.getChannel() + " :" + bot.getRandomResponse() + "\r\n";
                sendMessage(sockfd, response);
            }
        }
    }

    close(sockfd);
    return 0;
}
