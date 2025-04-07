#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>

class Server
{
    private:
        int port;
        std::string password;
        int server_fd;
        int epoll_fd; // Ajout de epoll_fd comme membre de la classe

    public:
        Server(int port, const std::string& password);
        void run();
};

#endif
