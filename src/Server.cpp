/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aauberti <aauberti@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/06 12:48:35 by jlorette          #+#    #+#             */
/*   Updated: 2025/04/07 08:36:27 by aauberti         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Server.hpp>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

Server::Server(int port, const std::string& password)
    : port(port), password(password), server_fd(-1) {}

void Server::run()
{
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return;
    }

    // Configuring the socket to reuse the address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "Error configuring socket: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    // Server Address Configuration
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Accepts connections from any interface
    address.sin_port = htons(port);

    // Binding socket to address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Error during bind: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    // Socket listening (with a queue of 10 connections)
    if (listen(server_fd, 10) < 0)
    {
        std::cerr << "Error while listening: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    std::cout << "Server started on port " << port << std::endl;
    std::cout << "Waiting for connections..." << std::endl;

    // Loop to accept connections
    while (true)
    {
        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0)
        {
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
            continue;
        }

        std::cout << "New connection accepted!" << std::endl;

        // Send a welcome message to the customer
        const char* welcome = "220 Welcome to the IRC server!\r\n";
        send(client_fd, welcome, strlen(welcome), 0);

        // Communication loop with the client
        char buffer[1024];
        while (true)
        {
            memset(buffer, 0, sizeof(buffer));
            int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

            if (bytes_received <= 0)
            {
                // Client disconnected or error
                if (bytes_received == 0)
                    std::cout << "Client disconnected" << std::endl;
                else
                    std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
                break;
            }

            // // Process the received message
            // std::cout << "Received: " << buffer;

            // // Echo back the message (simple response)
            // std::string response = "Server echo: ";
            // response += buffer;
            // send(client_fd, response.c_str(), response.length(), 0);
            std::string msg(buffer);

            if (msg.find("JOIN") == 0)
            {
                std::string channel = msg.substr(5);
                size_t newline = channel.find_first_of("\r\n");
                if (newline != std::string::npos)
                    channel = channel.substr(0, newline);

                if (channel[0] != '#')
                    channel = "#" + channel;

                std::string nick = "aauberti"; // definir l'username correctement apres ! juste je fais des test la 
                std::string join_reply;

                join_reply += ":" + nick + "!~user@localhost JOIN :" + channel + "\r\n";
                join_reply += ":ircserv 332 " + nick + " " + channel + " :Welcome to the channel!\r\n";
                join_reply += ":ircserv 353 " + nick + " = " + channel + " :" + nick + "\r\n";
                join_reply += ":ircserv 366 " + nick + " " + channel + " :End of /NAMES list.\r\n";

                send(client_fd, join_reply.c_str(), join_reply.length(), 0);
            }
            else
            {
                std::string response = "Server echo: ";
                response += buffer;
                send(client_fd, response.c_str(), response.length(), 0);
            }
        }

        // Close connection after client disconnects or error
        close(client_fd);
    }
}
