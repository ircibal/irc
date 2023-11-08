#include "Client.hpp"
#include "Channel.hpp"

Client::Client() : _pass(false) {
	_nickname = "";
	_hostname = "";
	_username = "";
	_socket_fd = -1;
	_realname = "";
	_channel_list = std::map<std::string, Channel *>();
	_channel_limit = CLIENT_CHANNEL_LIMIT;
}

Client::Client(int socket_fd) : _socket_fd(socket_fd), _pass(false) {
	_nickname = "";
	_hostname = "";
	_username = "";
	_realname = "";
	_channel_limit = CLIENT_CHANNEL_LIMIT;
	_channel_list = std::map<std::string, Channel *>();
}

Client::Client(int socket_fd, std::string nickname, std::string username,
std::string hostname, std::string realname, std::string user_ip)
: _socket_fd(socket_fd), _nickname(nickname), _username(username), _hostname(hostname),
_realname(realname), _user_ip(user_ip), _pass(false) {
	_channel_list = std::map<std::string, Channel *>();
	_channel_limit = CLIENT_CHANNEL_LIMIT;
}

Client::~Client() {}

int	Client::getSocketFd() const { return _socket_fd; };

const std::string&	Client::getNickname() const { return _nickname; };

const std::string&	Client::getUsername() const { return _username; };

const std::string&	Client::getHostname() const { return _hostname; };

const std::string&	Client::getRealname() const { return _realname; };

const std::string&	Client::getUserIp() const { return _user_ip; };

bool	Client::getPass() const { return _pass; };

int	Client::getChannelLimit() const { return _channel_limit; };

const std::map<std::string, Channel *>&	Client::getChannelList() const { return _channel_list; };

void	Client::setSocketFd(int socket_fd) { _socket_fd = socket_fd; };

void	Client::setNickname(const std::string& nickname) { _nickname = nickname; };

void	Client::setUsername(const std::string& username) { _username = username; };

void	Client::setHostname(const std::string& hostname) { _hostname = hostname; };

void	Client::setRealname(const std::string& realname) { _realname = realname; };

void	Client::setUserIp(const std::string& user_ip) { _user_ip = user_ip; };

void	Client::setChannelLimit(int new_limit) { _channel_limit = new_limit; };

void	Client::setPass() { _pass = true; };

void	Client::addChannelList(Channel *channel) {
	_channel_list.insert(std::pair<std::string, Channel *>(channel->getChannelName(), channel));
}

int	Client::checkChannelLimit() const{
	int cnt = _channel_list.size();
	if (cnt < _channel_limit)
		return 0;
	return -1;
}

void	Client::removeChannelList(Channel *channel) { _channel_list.erase(channel->getChannelName()); }

std::string	Client::getPrefix() const {
	std::string username = "!" + _username;
	std::string hostname = "@" + _hostname;
	return _nickname + username + hostname;
}
