#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include "util.h"

class Server;

class Client;

class Channel {
	public :
		Channel(const std::string& name, Client *client);
		~Channel();

		// Getter
		const std::string&	getChannelName() const;
		const std::string&	getChannelPassword() const;
		const std::string&	getChannelTopic() const;
		const std::set<char>&	getChannelMode() const;
		size_t	getUserLimit() const;
		std::map<int, Client *>::size_type	getUserCount() const;
		const std::map<int, Client *>&	getUserList() const;
		const std::map<int, Client *>&	getInviteList() const;
		const std::map<int, Client *>&	getChannelOperator() const;
		const std::string	getUserNameList();

		// Setter
		void	setChannelName(std::string new_name);
		void	setChannelPassword(std::string new_password);
		void	removeChannelPassword();
		void	setChannelTopic(std::string new_topic);
		std::vector<std::string>	setChannelMode(std::vector<std::string> token, Client *client);
		void	setUserLimit(long new_limits);

		// Mode
		void	clearChannelMode();
		std::vector<std::string>	getChannelModeParams() const;
		bool	checkChannelMode(char c);

		// Operator
		bool	isChannelOperator(Client *client);
		Client	*findChannelOperator(std::string nickname);
		bool	addChannelOperator(Client *new_operator);
		bool	removeChannelOperator(Client *old_operator);
		void	clearChannelOperator();

		// User
		bool	isChannelUser(Client *client);				// 유저 목록에 있는지 확인
		bool	isChannelUser(std::string nickname);
		Client	*findChannelUser(Client *client);			// 유저 목록에서 검색
		Client	*findChannelUser(std::string nickname);
		bool	addChannelUser(Client *client);				// 유저 목록에 추가
		bool	removeChannelUser(Client *client);			// 유저 목록에서 지우기
		void	clearUserList();

		// Invite
		bool	isInvitedUser(Client *client);				// 초대 목록에 있는지 확인
		Client	*findInvitedUser(std::string nickname);	// 초대 목록에서 검색
		bool	addInvitedUser(Client *client);				// 초대 목록에 추가
		bool	removeInvitedUser(Client *client);			// 채널 목록에서 지우기
		void	clearInviteList();

		// Check
		int	checkUserLimit() ;
		int	checkInvite(Client *client);
		int	checkInvite(int fd);
		int	checkPassword(std::string password);

		class ChannelModeException : public std::exception {
			public :
				ChannelModeException(const std::string& message);
				~ChannelModeException() throw();
				virtual const char	*what() const throw();
			private:
				std::string _message;

				ChannelModeException();
		};

	private :
		std::string	_name;						// 채널 이름
		std::string	_password;					// 채널 비밀번호
		std::string	_topic;						// 채널 토픽
		std::set<char>	_mode;					// 채널 모드
		size_t	_user_limit;					// 최대 유저 인원수
		std::map<int, Client *>	_user_list;		// 유저 목록
		std::map<int, Client *>	_invite_list;	// 유저 초대 목록
		std::map<int, Client *>	_operator;		// 채널 오퍼레이터

		Channel();
		Channel(const Channel& copy);
		Channel& operator=(const Channel& assign);

		void	initialize();
};

#endif
