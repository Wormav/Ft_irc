#include <Command.hpp>
#include <sys/socket.h>

void Command::handlePing(int client_fd, std::istringstream& iss) {
    std::string token;
    iss >> token;
    std::string response = "PONG " + token + "\r\n";
    send(client_fd, response.c_str(), response.length(), 0);
}
