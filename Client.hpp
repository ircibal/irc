#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "util.h"

class Channel;

class Client {
	public :
		Client();
		Client(int socket_fd);
		Client(int socket_fd, std::string nickname, std::string username,
			std::string hostname, std::string realname, std::string user_ip);
		~Client();

		// Getter
		int	getSocketFd() const;
		const std::string&	getNickname() const;
		const std::string&	getUsername() const;
		const std::string&	getHostname() const;
		const std::string&	getRealname() const;
		const std::string&	getUserIp() const;
		const std::map<std::string, Channel *>&	getChannelList() const;
		int	getChannelLimit() const;

		// Setter
		void	setSocketFd(int socket_fd);
		void	setPass();
		void	setNickname(const std::string& nickname);
		void	setUsername(const std::string& username);
		void	setHostname(const std::string& hostname);
		void	setRealname(const std::string& realname);
		void	setUserIp(const std::string& user_ip);
		void	addChannelList(Channel *channel);
		void	removeChannelList(Channel *channel);
		void	setChannelLimit(int new_limit);

		bool	getPass() const;
		std::string	getPrefix() const;
		int	checkChannelLimit() const; //-1불가 0가능 : 유저가 가입할 수 있는 최대 채널 갯수 초과 체크

	private :
		int	_socket_fd;
		std::string	_nickname;
		std::string	_username;
		std::string	_hostname;
		std::string	_realname;
		std::string	_user_ip; // 유저의 IP주소
		std::map<std::string, Channel *>	_channel_list; // 유저가 가입한 채널 목록
		int	_channel_limit;
		bool	_pass;

		Client(const Client& copy);
		Client&	operator=(const Client& obj);
};

#endif
