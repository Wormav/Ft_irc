#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include "../includes/Bot.hpp"
#include "../includes/Server.hpp"

#define BUFFER_SIZE 1024

// Structure pour passer les paramètres au thread du bot
struct BotParams {
    int port;
    std::string password;
};

int connectToServer(int port) {
    // Attendre un peu que le serveur soit lancé
    sleep(2);

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

// Fonction qui sera exécutée par le thread du bot
void* runBot(void* arg) {
    BotParams* params = static_cast<BotParams*>(arg);

    Bot bot("IRCBot", "bot", "IRC Bot", "#bot");

    int sockfd = connectToServer(params->port);
    if (sockfd < 0) {
        delete params; // Nettoyer la mémoire
        return NULL;
    }

    registerBot(sockfd, bot, params->password);

    sleep(2);
    joinChannel(sockfd, bot.getChannel());

    char buffer[BUFFER_SIZE];
    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    std::cout << "Bot connecté au serveur IRC" << std::endl;

    while (true) {
        int ret = poll(fds, 1, 1000); // Timeout de 1 seconde pour permettre une sortie propre

        if (ret < 0) {
            std::cerr << "Erreur de poll" << std::endl;
            break;
        }

        if (ret == 0) {
            // Timeout, continuer la boucle
            continue;
        }

        if (fds[0].revents & POLLIN) {
            ssize_t bytesRead = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (bytesRead <= 0) {
                std::cerr << "Connexion fermée ou erreur" << std::endl;
                break;
            }

            buffer[bytesRead] = '\0';
            std::cout << "Bot a reçu: " << buffer << std::endl;

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
    delete params; // Nettoyer la mémoire
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    std::string password = argv[2];

    // Créer un thread pour le bot
    pthread_t bot_thread;
    BotParams* params = new BotParams;
    params->port = port;
    params->password = password;

    // Lancer le thread du bot
    if (pthread_create(&bot_thread, NULL, runBot, params) != 0) {
        std::cerr << "Erreur lors de la création du thread du bot" << std::endl;
        delete params;
        return 1;
    }

    // Configurer le thread pour qu'il se termine automatiquement lorsque le thread principal se termine
    pthread_detach(bot_thread);

    // Lancer le serveur (dans le thread principal)
    std::cout << "Démarrage du serveur IRC..." << std::endl;
    Server server(port, password);
    server.run();  // Cette fonction est bloquante jusqu'à ce que le serveur s'arrête

    // On n'atteint ce point que si le serveur s'arrête
    std::cout << "Serveur IRC arrêté" << std::endl;

    // Attendre un peu que le thread du bot puisse terminer proprement
    sleep(1);

    return 0;
}
