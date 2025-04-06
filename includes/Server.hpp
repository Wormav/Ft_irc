/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jlorette <jlorette@42angouleme.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/06 12:47:58 by jlorette          #+#    #+#             */
/*   Updated: 2025/04/06 13:00:05 by jlorette         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>

class Server
{
    public:
        Server(int port, const std::string& password);
        void run();

    private:
        int port;
        std::string password;
        int server_fd;
};

#endif
