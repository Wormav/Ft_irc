/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jlorette <jlorette@42angouleme.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/07 10:32:07 by jlorette          #+#    #+#             */
/*   Updated: 2025/04/07 10:37:22 by jlorette         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <string>

class Client
{
	private:
		int         _socket;
		std::string _nickname;
		std::string _username;
		std::string _realname;
		bool        _isRegistered;
		bool        _isOperator;

	public:
		Client(int socket);
		~Client();

		// Getters
		int         getSocket() const;
		std::string getNickname() const;
		std::string getUsername() const;
		std::string getRealname() const;
		bool        isRegistered() const;
		bool        isOperator() const;

		// Setters
		void setNickname(const std::string& nickname);
		void setUsername(const std::string& username);
		void setRealname(const std::string& realname);
		void setRegistered(bool status);
		void setOperator(bool status);
};

#endif
