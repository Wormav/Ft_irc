#include <Command.hpp>
#include <sys/socket.h>

void Command::handlePass(int client_fd, std::istringstream& iss) {
    std::string pass;
    if (std::getline(iss, pass) && !pass.empty()) {
        if (pass[0] == ' ') pass = pass.substr(1);
        size_t crlf = pass.find_first_of("\r\n");
        if (crlf != std::string::npos) {
            pass = pass.substr(0, crlf);
        }

        if (pass == password) {
            users[client_fd].setPasswordVerified(true);
        } else {
            std::string error = ":ircserv 464 * :Password incorrect\r\n";
            send(client_fd, error.c_str(), error.length(), 0);
        }
    }
}
