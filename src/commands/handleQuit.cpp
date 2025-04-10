#include <Command.hpp>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

void Command::handleQuit(int client_fd, const std::string& line) {
    // Vérifier si l'utilisateur existe encore
    if (users.find(client_fd) == users.end()) {
        return;
    }

    std::string quit_message = "Quit";
    size_t pos = line.find(" :");
    if (pos != std::string::npos) {
        quit_message = line.substr(pos + 2);
    }

    // Récupérer le nom d'utilisateur avant de faire des modifications
    std::string username = users[client_fd].getFullIdentity();

    // Créer la notification de départ
    std::string quit_notification = ":" + username + " QUIT :Quit: " + quit_message + "\r\n";

    // Collecter les canaux dont l'utilisateur est membre
    std::vector<std::string> channelsToProcess;
    for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); ++it) {
        if (it->second.hasMember(client_fd)) {
            channelsToProcess.push_back(it->first);
        }
    }

    // Traiter chaque canal séparément pour éviter les problèmes d'itérateurs
    for (size_t i = 0; i < channelsToProcess.size(); ++i) {
        std::map<std::string, Channel>::iterator channel_it = channels.find(channelsToProcess[i]);

        if (channel_it == channels.end()) {
            continue; // Le canal a peut-être été supprimé entre-temps
        }

        // Traiter le cas où l'utilisateur est le dernier opérateur
        if (channel_it->second.isOperator(client_fd)) {
            // Compter les opérateurs (excluant celui qui part)
            int remainingOps = 0;
            const std::set<int>& ops = channel_it->second.getOperators();
            for (std::set<int>::const_iterator op_it = ops.begin(); op_it != ops.end(); ++op_it) {
                if (*op_it != client_fd) {
                    remainingOps++;
                }
            }

            // S'il n'y aura plus d'opérateurs mais qu'il reste des membres
            if (remainingOps == 0 && channel_it->second.getMembers().size() > 1) {
                // Trouver un nouveau membre à promouvoir
                int newOp = -1;
                const std::set<int>& members = channel_it->second.getMembers();
                for (std::set<int>::const_iterator mem_it = members.begin(); mem_it != members.end(); ++mem_it) {
                    if (*mem_it != client_fd) {
                        newOp = *mem_it;
                        break;
                    }
                }

                // Promouvoir le nouveau membre comme opérateur
                if (newOp != -1 && users.find(newOp) != users.end()) {
                    channel_it->second.addOperator(newOp);
                    std::string mode_msg = ":ircserv MODE " + channelsToProcess[i] + " +o " + users[newOp].getNickname() + "\r\n";
                    channel_it->second.broadcastMessage(mode_msg);
                }
            }
        }

        // Envoyer la notification de départ aux autres membres
        channel_it->second.broadcastMessage(quit_notification, client_fd);

        // Retirer l'utilisateur du canal
        channel_it->second.removeMember(client_fd);

        // Supprimer le canal s'il est vide
        if (channel_it->second.isEmpty()) {
            channels.erase(channel_it);
        }
    }

    // Retirer l'utilisateur de la map des utilisateurs
    users.erase(client_fd);

    // N'oubliez pas de fermer le socket!
    if (client_fd > 0) {
        close(client_fd);
    }
}
