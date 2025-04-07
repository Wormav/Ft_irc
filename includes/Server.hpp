/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jlorette <jlorette@42angouleme.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/06 12:47:58 by jlorette          #+#    #+#             */
/*   Updated: 2025/04/07 10:41:51 by jlorette         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>
#include <Client.hpp>

class Server
{
    public:
        Server(int port, const std::string& password);
        void run();

    private:
        int port;
        std::string password;
        int server_fd;
        std::map<int, Client> clients; // Map des clients (clé: socket_fd, valeur: objet Client)
};

#endif
