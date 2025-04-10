#include <Command.hpp>
#include <Server.hpp>
#include <sys/socket.h>

void Command::handleQuit(int client_fd, std::istringstream& iss) {
    std::string quit_message = "Quit";

    std::string rest;
    if (std::getline(iss, rest) && !rest.empty()) {
        size_t colon_pos = rest.find(':');
        if (colon_pos != std::string::npos) {
            quit_message = rest.substr(colon_pos + 1);
        } else if (rest[0] == ' ') {
            quit_message = rest.substr(1);
        }
    }

    std::string quit_response = "ERROR :Closing Link: localhost (" + quit_message + ")\r\n";
    send(client_fd, quit_response.c_str(), quit_response.length(), 0);

    server->disconnectClient(client_fd);
}
