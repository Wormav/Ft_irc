#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>
#include <set>
#include <signal.h>
#include <User.hpp>
#include <Channel.hpp>
#include <Command.hpp>

class Server
{
	private:
		int port;
		std::string password;
		int server_fd;
		int epoll_fd;
		static bool running;

		std::map<int, std::string> client_buffers;
		std::map<int, User> users;
		std::map<std::string, Channel> channels;
		Command* command_handler;

		void setupSocket();
		void handleNewConnection();
		void handleClientData(int client_fd);
		void processCommand(int client_fd, const std::string& line);

		static void handleSignal(int signal);

	public:
		Server(int port, const std::string& password);
		~Server();

		void run();
		void disconnectClient(int client_fd);
		void cleanupResources();
};

#endif
