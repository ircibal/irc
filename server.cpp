#include "server.hpp"
#include "message.h"
#include "util.h"
#include <iterator>
#include <ostream>
#include <set>
#include <sstream>
#include <utility>
#include <vector>
//10.11.3.2
//irssi -c 10.28.3.5 -p 8080 -w 1234 -n juhyulee
//irssi -c 10.12.1.7 -p 8080 -w 1234 -n juhyulee
//서버네임 숫자 닉네임 메세지
//두명 있을때 한명 나가면 서버도 꺼짐

void Server::send_msg(std::string msg, int fd) { //메세지 전송하는 함수
	std::cout << msg << std::endl;
	send(fd, msg.c_str(), msg.size(), 0);
}

void Server::broadcastChannelMessage(std::string message, int send_fd) {
	(void)send_fd; ///????
	for (std::map<int,Client>::iterator iter = this->usrlist.begin();
	iter != this->usrlist.end(); iter++) {
		send_msg(message, iter->first);
	}
}

void Server::read_msg(std::string msg, int fd) { // 메세지 읽는 함수
	std::vector<std::string> token;
	std::istringstream iss(msg);
	std::string line;

	while (std::getline(iss, line))
		token.push_back(line);
	for (size_t i = 0; i < token.size(); i++) {
		std::cout << token[i] << std::endl;
		handle_cmd(token[i], fd); //개행 기준으로 tokenizer해서 들어감
	}
}

void Server::handle_cmd(std::string cmd, int fd) { // 메세지 파싱하는 함수
	std::vector<std::string> token;
	std::istringstream iss(cmd);
	std::string word;
	int paramcnt = 0;
	Client user = this->usrlist[fd];

	while (iss >> word) {
		token.push_back(word);
		paramcnt += 1;
	}
	std::cout << "token: " << token[0] << std::endl;
	if (token[0] == "PASS") {
		if (paramcnt != 2) {
			send_msg(ERR_NEEDMOREPARAMS(usrlist[fd].username, "PASS"), fd);
		}
		if (token[1] == serverpassword) {
			usrlist[fd].pass += 1;
			usrlist[fd].fd = fd;
		}
		else if (token[1] != serverpassword) {
			send_msg(ERR_PASSWDMISMATCH(usrlist[fd].username), fd);
			close(fd);
		}
	}
	else if (token[0] == "NICK") {
		std::map<int, Client>::iterator it;
		for (it = usrlist.begin(); it != usrlist.end(); ++it) {
			if (it->second.nickname == token[1]) {
				send_msg(ERR_NICKNAMEINUSE(usrlist[fd].username), fd);
			}
		}
		usrlist[fd].nickname = token[1];
		usrlist[fd].pass += 1;
	}
	else if (token[0] == "USER") { //생성자로 대체
		if (paramcnt != 5) {
			send_msg(ERR_NEEDMOREPARAMS(usrlist[fd].username, "USER"), fd);
		}
		usrlist[fd].username = token[1];
		usrlist[fd].hostname = token[2];
		usrlist[fd].servername = token[3];
		usrlist[fd].realname = token[4];
		usrlist[fd].pass += 1;
		checkuserinformation(usrlist[fd]);
	}

	else if (token[0] == "PING") {
		std::string pong = "irc_test\r\n";
		send_msg(RPL_PONG(usrlist[fd].username, pong),fd);
	}
	else if (token[0] == "JOIN") {
		if (this->search_channel(token[1]) == NULL) {
			make_channel(token[1]);
			std::cout << "token: " << token[1] << std::endl;
		}
		clist[token[1]].adduser(fd, usrlist[fd]);
		if (clist[token[1]].usrlist.size() == 1) {
			clist[token[1]].addchanneloperator(usrlist[fd]);
		}
		std::map<int,Client> clients = this->clist[token[1]].usrlist;
		std::map<std::string, Client> channeloperator = this->clist[token[1]].getchanneloperator();
		std::string s_users;
		for (std::map<int, Client>::iterator iter = clients.begin(); iter != clients.end(); iter++)
		{
			//operator 여러 명일 수 있음. 확인하는 함수로 바꿀 것
			if (channeloperator.find(iter->second.nickname) != channeloperator.end()) {
				s_users.append("@");
				s_users.append(iter->second.nickname + " ");
			}
		}

		for (std::map<int,Client>::iterator iter = clients.begin();
		iter != clients.end(); iter++) {
			this->send_msg(RPL_JOIN(clients[fd].nickname, token[1]), iter->first);
		}
		//topic 있을 경우 RPL_TOPIC, RPL_TOPICWHOTIME send
		this->send_msg(RPL_NAMREPLY(clients[fd].nickname, '=', token[1], s_users), fd);
		this->send_msg(RPL_ENDOFNAMES(clients[fd].nickname, token[1]), fd);
	}
	else if (token[0] == "PART") { //채널나가는명령어
		for (size_t i = 1; i <= token.size(); i++) {
			Channel	targetChannel = this->clist[token[i]];
			targetChannel.deleteuser(fd);
			if (targetChannel.usrlist.size() == 0) {
				this->clist.erase(clist.find(token[i]));
			}
		}
	}
	else if (token[0] == "NOTICE") {//메세지전송
		Channel *target_channel = this->search_channel(token[1]);
		std::string send_message;
		for (size_t i = 0; i + 2 < token.size(); i++) {
			if (i != 0) {
				send_message += " ";
			}
			send_message += token[i + 2];
		}
		for (std::map<int, Client>::iterator iter = target_channel->usrlist.begin(); \
			iter != target_channel->usrlist.end(); ++iter) {
			this->send_msg(send_message, iter->second.fd);
		}
	}
	else if (token[0] == "PRIVMSG") { //메세지 전송
		if (token[2][1] == '$') {
			Client *receive_client = this->search_user(token[2].substr(2, token[2].size()));
			if (receive_client == NULL) {
				// Does not exist client
			}
			int	receive_fd = receive_client->fd;
			std::string send_message;
			for (size_t i = 0; i + 3 < token.size(); i++) {
				if (i != 0) {
					send_message += " ";
				}
				send_message += token[i + 3];
			}
			Client send_client = this->usrlist[fd];
			this->send_msg(RPL_PRIVMSG(send_client.getPrefix(), token[1], send_message), receive_fd);
			this->send_msg(send_message, receive_fd);
		}
		else {
			std::string send_message;
			for (size_t i = 0; i + 2 < token.size(); i++) {
				if (i != 0) {
					send_message += " ";
				}
				send_message += token[i + 2];
			}
			Channel *target_channel = this->search_channel(token[1]);
			std::map<int,Client> clients = target_channel->usrlist;
			for (std::map<int,Client>::iterator iter = clients.begin();
			iter != clients.end(); iter++) {
				if (iter->second.fd != fd) {
					this->send_msg(RPL_PRIVMSG(clients[fd].getPrefix(), token[1], send_message), iter->second.fd);
				}
			}
		}
	}
	else if (token[0] == "KICK") { //채널 방출
		Channel *target_channel = this->search_channel(token[1]);
		Client *target_user = target_channel->search_user(token[2]);
		target_channel->deleteuser(target_user->fd);
		std::stringstream kick_message;
		kick_message << target_channel->getchannelname() << ". kicked from the channel.";
		if (token.size() < 3) {
			kick_message << " : " << token[3];
		}
		kick_message << std::endl;
		this->send_msg(kick_message.str(), target_user->fd);
		//리플라이 + and 유저가 없는 유저 킥했을때 예?왜?처리
	}
	else if (token[0] == "INVITE") { //채널에 유저 초대 Parameters: <nickname> <channel>
		std::cout << "invite ok" << token.size() <<std::endl;
		// ERR_NEEDMOREPARAMS: 유효하지 않은 명령어
		if (token.size() != 3) {
			this->send_msg(ERR_NEEDMOREPARAMS(token[0], token[0]), fd);
			return ;
		}

		// ERR_NOSUCHNICK: 존재하지 않는 유저
		Client *invite_client = this->search_user(token[1]);
		if (invite_client == NULL) {
			std::string str = "401 " + user.username + " "+ token[1] + " :No such nick" + "\r\n";
			std::cout<< ": irc.local " + str<< std::endl;
			
			this->send_msg(": irc.local " + str, fd);
			return ;
		}

		// ERR_NOSUCHCHANNEL: 존재하지 않는 채널
		Channel *invite_channel = this->search_channel(token[2]);
		if (invite_channel == NULL) {
			this->send_msg(ERR_NOSUCHCHANNEL(user.getPrefix(), token[2]), fd);
			return ;
		}

		//ERR_USERONCHANNEL: 채널에 이미 존재하는 유저 user? nick channel
		if (invite_channel->search_user(token[1])){
			this->send_msg(ERR_USERONCHANNEL(user.getPrefix(), token[1], token[2]), fd);
			return ;
		}

		//ERR_NOTONCHANNEL : 현재 내가 해당 채널에 없음
		if (invite_channel->search_user(user.nickname) == NULL){
			this->send_msg(ERR_USERONCHANNEL(user.username, user.nickname, token[2]), fd);
			return ;
		}
		//유저 오퍼레이터 권한 확인 후 최종 초대 RPL_INVITING
		std::map<std::string, Client> invite_channel_operator = invite_channel->getchanneloperator();
		for (std::map<std::string, Client>::iterator  iter = invite_channel_operator.begin(); \
			iter != invite_channel_operator.end(); iter++){
			if (iter->second.nickname == user.nickname)
			{ 
				//초대된 목록에만 들어감.
				invite_channel->adduser(invite_client->fd, *invite_client);
				this->send_msg(RPL_INVITING(user.nickname, token[1], token[2]), fd);
				//다른 사람들, 초대된 사람한테 메세지 보내기 필요
				return ;
			}
		}
		//ERR_CHANOPRIVSNEEDED2 : 채널 부 오퍼레이터 권한 없음------channel에 부 오퍼레이터 없음
		//RPL_AWAY -----------------------message.h에 없음

		//ERR_CHANOPRIVSNEEDED : 채널 오퍼레이터 권한 없음
		this->send_msg(ERR_CHANOPRIVSNEEDED(user.nickname, token[2]), fd);
		return ;
	}
	else if (token[0] == "TOPIC") { //토픽 + 채널명 - 토픽띄움 / 토픽 + 채널명 + 변경토픽 - 토픽변경
		std::map<int,Client> clients = this->clist[token[1]].usrlist;
		if (token.size() == 2) { //토픽 띄움
			//채널이 없으면 ERR_NOSUCHCHANNEL
			//
			std::string view_topic = this->search_channel(token[1])->getchanneltopic();
			if (view_topic.empty())
				send_msg(RPL_NOTOPIC(clist[token[1]].usrlist[fd].nickname,token[1]),fd);
			else
			{
				send_msg(RPL_TOPIC(usrlist[fd].nickname,token[1],token[2]),fd);
				send_msg(RPL_TOPICWHOTIME(usrlist[fd].nickname, token[1],usrlist[fd].nickname, "0"), fd);
			}
		}
		else if (token.size() == 3) { // 토픽 변경
			//오퍼레이터 확인
			Channel *target_channel = this->search_channel(token[1]);
			target_channel->setchanneltopic(token[2]);
			// 채널에 있는 모든 유저에게 broadcast
			for (std::map<int,Client>::iterator iter = clients.begin();
				iter != clients.end(); iter++) {
				this->send_msg(RPL_MY_TOPIC(usrlist[fd].nickname, token[1], token[2]), iter->first);
			}
		}
	}
	else if (token[0] == "MODE") { //채널모드설정
		Channel *channel = this->search_channel(token[1]);
		if (channel == NULL) {
			// Does not exist channel
			return ;
		}
		if (token.size() == 2) {
			// View channel mode
			std::string disp_mode;
			std::string params;
			for (std::set<char>::iterator iter = channel->getchannelmode().begin();
			iter != channel->getchannelmode().end(); iter++) {
				if (iter == channel->getchannelmode().begin() && iter != channel->getchannelmode().end()) {
					disp_mode += "+";
				}
				disp_mode += *iter;
				if (*iter == 'k' || *iter == 'l') {
					params += " ";
					if (*iter == 'k') {
						params += channel->getchannelpassword();
					}
					else {
						params += channel->getusrlimits();
					}
				}
			}
			broadcastChannelMessage(RPL_CHANNELMODEIS(this->usrlist[fd].getPrefix(), token[1], disp_mode, params), fd);
			// broadcastChannelMessage(RPL_CHANNELMODEIS(this->usrlist[fd].getPrefix(), token[1], disp_mode, params));
			// for (std::map<int,Client>::iterator iter = channel->usrlist.begin();
			// iter != channel->usrlist.end(); iter++) {
				// send_msg(RPL_MODE(this->usrlist[fd].getPrefix(), token[1], disp_mode, params), iter->first);
			// }
			return ;
		}
		if (token[2][0] != '+' && token[2][0] != '-' && token[2].size() != 2) {
			return ;
		}
		std::vector<std::string> *mode_params = channel->setchannelmode(*this, token);
		if (mode_params) {
			broadcastChannelMessage(RPL_MODE(this->usrlist[fd].getPrefix(), channel->getchannelname(), *mode_params[0].data(), *mode_params[1].data()), fd);
			// broadcastChannelMessage(RPL_MODE(this->usrlist[fd].getPrefix(), channel->getchannelname(), *mode_params[0].data(), *mode_params[1].data()));
		}
	}
	else if (token[0] == "QUIT") { //다른 유저들한테 나갔다고 보냄
		std::string quit_message = usrlist[fd].nickname + " :Quit\r\n";
		std::set<int> receive_user;
		for (std::map<int, Client>::iterator iter = usrlist.begin(); iter != usrlist.end(); iter++) {
			if (iter->first != fd) {
				receive_user.insert(iter->first);
			}
		}
		for (std::set<int>::iterator iter = receive_user.begin(); iter != receive_user.end(); iter++) {
			send_msg(quit_message, *iter);
			std::cout << "iter test: " << *iter << std::endl;
		}
		close(fd);
	}
}

void Server::checkuserinformation(Client& user) {
	std::cout << "pass " << user.nickname << std::endl;
	if (user.pass == 3) {
		std::string welcome = ":irc_test 001 " + user.nickname + " :Welcome to the irc Network, " + user.nickname + "!root@" + user.servername + "\r\n";
		std::cout << welcome << std::endl;
		send_msg(welcome, user.fd);
	}
}

void Server::make_channel(std::string channelname) {
	Channel newchannel;

	newchannel.setchannelname(channelname);
	clist[channelname] = newchannel;
}

Channel *Server::search_channel(std::string channelname) {
	for (std::map<std::string, Channel>::iterator iter = this->clist.begin(); iter != this->clist.end(); ++iter) {
		if (iter->second.getchannelname() == channelname)
		return &iter->second;
	}
	return 0;
}

Client *Server::search_user(std::string nickname) {
	for (std::map<int, Client>::iterator iter = this->usrlist.begin(); iter != this->usrlist.end(); ++iter) {
		if (iter->second.nickname == nickname)
		return &iter->second;
	}
	return 0;
}

void Server::adduser(Client user, int fd) {
	usrlist[fd] = user;
}

void Server::deluser(Client user, int fd) {
	(void)user; ///???
	usrlist.erase(usrlist.find(fd));
}

// std::map<std::string, ICommand*> command;

// command[token[0]]->run(token);

// class ICommand
// {

// };
