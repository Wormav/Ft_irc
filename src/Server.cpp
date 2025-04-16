#include <Server.hpp>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <sys/epoll.h>
#include <sstream>
#include <algorithm>
#include <signal.h>
#include <vector>

bool Server::running = true;

void Server::handleSignal(int signal)
{
    if (signal == SIGINT)
	{
		std::cout << "\nReceiving SIGINT (Ctrl+C). Shutting down server..." << std::endl;
        running = false;
    }
}

Server::Server(int port, const std::string& password)
    : port(port), password(password), server_fd(-1), epoll_fd(-1)
{
    command_handler = new Command(this, users, channels, password);

    struct sigaction sa;
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}

Server::~Server()
{
    cleanupResources();
}

void Server::cleanupResources()
{
    std::vector<int> client_fds;
    for (std::map<int, User>::iterator it = users.begin(); it != users.end(); ++it)
        client_fds.push_back(it->first);

    for (size_t i = 0; i < client_fds.size(); ++i)
        disconnectClient(client_fds[i]);

    if (server_fd >= 0)
	{
        close(server_fd);
        server_fd = -1;
    }

    if (epoll_fd >= 0)
	{
        close(epoll_fd);
        epoll_fd = -1;
    }

    delete command_handler;
    command_handler = NULL;
}

void Server::setupSocket()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
	{
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error configuring socket: " << strerror(errno) << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }

    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
        std::cerr << "Error during bind: " << strerror(errno) << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }

    if (listen(server_fd, 10) < 0)
	{
        std::cerr << "Error while listening: " << strerror(errno) << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
	{
        std::cerr << "Error creating epoll instance: " << strerror(errno) << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0)
	{
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

void Server::handleNewConnection()
{
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0)
	{
        std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
        return;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0)
	{
        std::cerr << "Error adding client socket to epoll: " << strerror(errno) << std::endl;
        close(client_fd);
        return;
    }

    std::cout << "New connection accepted! Client fd: " << client_fd << std::endl;

    users[client_fd] = User();
    client_buffers[client_fd] = "";
}

void Server::handleClientData(int client_fd)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0)
	{
        if (bytes_received == 0)
            std::cout << "Client disconnected (fd: " << client_fd << ")" << std::endl;
        else
            std::cerr << "Error receiving data: " << strerror(errno) << std::endl;

        disconnectClient(client_fd);
        return;
    }

    client_buffers[client_fd] += buffer;

    std::string& buf = client_buffers[client_fd];
    size_t pos;
    while ((pos = buf.find("\r\n")) != std::string::npos)
	{
        std::string line = buf.substr(0, pos);
        buf = buf.substr(pos + 2);

        std::cout << "Received from client " << client_fd << ": " << line << std::endl;

        processCommand(client_fd, line);
    }
}

void Server::disconnectClient(int client_fd)
{
    if (users.find(client_fd) == users.end())
        return;

    if (!users[client_fd].getNickname().empty())
	{
        std::string quit_notification = ":" + users[client_fd].getFullIdentity() + " QUIT :Connection closed\r\n";

        std::vector<std::string> userChannels;
        for (std::map<std::string, Channel>::iterator channel_it = channels.begin();
             channel_it != channels.end();
             ++channel_it) {
            if (channel_it->second.hasMember(client_fd))
                userChannels.push_back(channel_it->first);
        }

        for (size_t i = 0; i < userChannels.size(); ++i)
		{
            std::map<std::string, Channel>::iterator channel_it = channels.find(userChannels[i]);
            if (channel_it != channels.end())
			{
                if (channel_it->second.isOperator(client_fd))
				{
                    int remainingOps = 0;
                    const std::set<int>& ops = channel_it->second.getOperators();
                    for (std::set<int>::const_iterator op_it = ops.begin(); op_it != ops.end(); ++op_it)
					{
                        if (*op_it != client_fd)
                            remainingOps++;
                    }

                    if (remainingOps == 0 && channel_it->second.getMembers().size() > 1)
					{
                        int newOp = -1;
                        const std::set<int>& members = channel_it->second.getMembers();
                        for (std::set<int>::const_iterator mem_it = members.begin(); mem_it != members.end(); ++mem_it)
						{
                            if (*mem_it != client_fd)
							{
                                newOp = *mem_it;
                                break;
                            }
                        }
                        if (newOp != -1 && users.find(newOp) != users.end())
						{
                            channel_it->second.addOperator(newOp);
                            std::string mode_msg = ":ircserv MODE " + userChannels[i] + " +o " + users[newOp].getNickname() + "\r\n";
                            channel_it->second.broadcastMessage(mode_msg);
                        }
                    }
                }

                channel_it->second.broadcastMessage(quit_notification, client_fd);
                channel_it->second.removeMember(client_fd);

                if (channel_it->second.isEmpty())
                    channels.erase(channel_it);
            }
        }
    }

    client_buffers.erase(client_fd);
    users.erase(client_fd);

    if (client_fd > 0) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        close(client_fd);
    }
}

void Server::processCommand(int client_fd, const std::string& line)
{
    command_handler->process(client_fd, line);
}

void Server::run()
{
    setupSocket();

    if (server_fd < 0 || epoll_fd < 0)
	{
        std::cerr << "Server setup failed. Exiting." << std::endl;
        return;
    }

    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];


    while (running)
	{
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, 100);

        if (n_events < 0) {
            if (errno == EINTR)
                continue;

            std::cerr << "Error in epoll_wait: " << strerror(errno) << std::endl;
            break;
        }

        for (int i = 0; i < n_events; i++)
		{
            if (events[i].data.fd == server_fd)
                handleNewConnection();
            else
                handleClientData(events[i].data.fd);
        }
    }

	std::cout << "Cleaning up resources before quitting..." << std::endl;
    cleanupResources();
}
