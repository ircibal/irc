#include "message.h"
#include "util.h"
#include "Server.hpp"

//int Client::checkLimit == -1  : 유저가 가입할 수 있는 최대 채널 갯수 초과 체크
//int Channel::checkUserLimit == -1  : 채널에 가입할 수 있는 최대 인원 초과 체크

// token 인자 중 합쳐서 string 만드는 것들 합쳐서 돌려주는 함수
std::string getTotalMessage(size_t start, std::vector<std::string> token){
	std::string	topic = "";
	size_t	tokensize = token.size();
	if (start > tokensize)
		return topic;
	if (token[start][0] == ':')
		token[start] = &(token[start][1]);
	for (size_t i = start; i < tokensize - 1; i++) {
		topic += token[i];
		topic += " ";
	}
	topic += token[tokensize - 1];
	if (topic == ":")
		return "";
	return topic;
}

void	Server::commandJoin(std::vector<std::string> token, Client * user, int fd){
	std:: string password = "";
	std:: string & channel_name = token[1];
	const int paramcnt = token.size();

	if (paramcnt >= 2) {
		password = token[2];
	}
	// error::no channel name
	if (paramcnt < 1) {
		this->sendMessage(ERR_NEEDMOREPARAMS(user->getNickname(), "JOIN"), fd);
		return ;
	}
	Channel *ch = this->searchChannel(channel_name);
	//------------------------------------------------------success::new channel
	if (!ch){
		std::cout << "new channel made : " << token[1] << std::endl;
		ch = makeChannel(token[1], user);
		this->sendMessage(RPL_JOIN(user->getPrefix(), channel_name),fd);
		if (ch->getChannelTopic() != ""){
			this->sendMessage(RPL_TOPIC(user->getPrefix(), channel_name, ch->getChannelTopic()), fd);
		}
		this->sendMessage(RPL_NAMREPLY(user->getPrefix(), "=", channel_name, ch->getUserNameList()), fd);
		this->sendMessage(RPL_ENDOFNAMES(user->getPrefix(), channel_name), fd);
		return ;

	}
	//------------------------------------------------------exist channel
	//------------------------------------------------------error::client exceed max channel cnt
	if (user->checkChannelLimit() == -1)
	{
		this->sendMessage(ERR_TOOMANYCHANNELS(user->getNickname(), "JOIN"), fd);
		return ;
	}
	// error::
	else if (ch->checkUserLimit() == -1) {
		sendMessage(ERR_CHANNELISFULL(user->getNickname(), channel_name), fd);
	}
	else if (ch->getChannelMode().find('i') != ch->getChannelMode().end() && ch->isInvitedUser(user) == false){
		this->sendMessage(ERR_INVITEONLYCHAN(user->getNickname(), channel_name), fd);
	}
	else if (ch->checkInvite(fd) == -1 && ch->checkPassword(password) == -1){
		sendMessage(ERR_BADCHANNELKEY(user->getNickname(), channel_name), fd);
	}
	// Success:: exist channel
	else {
		ch->addChannelUser(user);
		std::cout << "exist channel enter : " << token[1] <<"\nthis channel now has " << ch->getUserCount() << std::endl;
		this->sendMessage(RPL_JOIN(user->getPrefix(), channel_name),fd);
		this->broadcastChannelMessage(RPL_JOIN(user->getPrefix(), channel_name), ch, fd);
		if (ch->getChannelTopic() != ""){
			this->sendMessage(RPL_TOPIC(user->getPrefix(), channel_name, ch->getChannelTopic()), fd);
		}
		this->sendMessage(RPL_NAMREPLY(user->getPrefix(), "=", channel_name, ch->getUserNameList()), fd);
		this->sendMessage(RPL_ENDOFNAMES(user->getPrefix(), channel_name), fd);
	}
	return ;
}

void	Server::commandUser(std::vector<std::string> token, int fd)
{
	//USER "username" "hostname" "servername" :"realname"
	std::string realname = D_REALNAME;
	Client *user = this->searchClient(fd);
	const int paramcnt = token.size();

	if(user != NULL) // already in user_list
	{
		this->sendMessage(ERR_ALREADYREGISTERED(user->getNickname()), fd);
		return ;
	}
	if (paramcnt == 5 && token [4][0] == ':')
		realname = &(token[4][1]);
	else //param err
	{
		this->sendMessage(ERR_NEEDMOREPARAMS((std::string)"root", (std::string)"USER"), fd);
		return ;
	}

	user = this->searchTemp(fd);
	if (!user)
	{
		user = new Client(fd);
		this->_temp_list.insert(std::make_pair(fd, user));
	}
	user->setUsername(token[1]);
	user->setHostname(token[2]);
	user->setUserIp(token[3]);
	user->setRealname(realname);
};

void	Server::commandPass(std::vector<std::string> token, int fd){
	Client * user = NULL;
	user = this->_temp_list.find(fd)->second;
	if(this->searchClient(fd) != NULL){
		this->sendMessage(ERR_ALREADYREGISTERED(user->getNickname()), fd);
	}
	else if (token.size() != 2){
		this->sendMessage(ERR_NEEDMOREPARAMS((std::string)"root", (std::string)"PASS"), fd);
	}
	else if (this->getServerPassword() != token[1]){
		this->sendMessage(ERR_PASSWDMISMATCH((std::string)"root"), fd);
		//disconnect?
	}
	else if (user != this->_temp_list.end()->second) // pass correct && temp user exist
	{
		user->setPass();
	}
	else { // pass correct && temp user none
		user = new Client(fd);
		user->setPass();
		this->_temp_list.insert(std::make_pair(fd, user));
	}
	return ;
};



void	Server::commandNick(std::vector<std::string> token, int fd)
{
	Client *user = NULL;
	if (token.size() != 2){
		this->sendMessage(ERR_NEEDMOREPARAMS(user->getNickname(), (std::string)"NICK"), fd);
		return ;
	}
	if (this->searchClient(token[1])){
		this->sendMessage(ERR_NICKNAMEINUSE(token[1]), fd);
		return;
	};
	user = this->searchClient(fd);
	if (user != NULL)
	{
		this->sendMessage(RPL_NICK(user->getPrefix(), token[1]), fd);
		user->setNickname(token[1]);
		return;
	}
	user = this->searchTemp(fd);
	if (user != NULL)
	{
		user->setNickname(token[1]);
	}
	else
	{
		user = new Client(fd);
		user->setNickname(token[1]);
		this->_temp_list.insert(std::make_pair(fd, user));
	}
};


void	Server::commandPing(std::vector<std::string> token, Client * user,  int fd)
{
	// std::cout << "check pong" << RPL_PONG(user->getPrefix(), token[1]) << std::endl;
	if (token.size() != 2){
		this->sendMessage(ERR_NOORIGIN(user->getNickname()), fd);
	}
	else{
		this->sendMessage(RPL_PONG(user->getPrefix(), token[1]), fd);
	}
}

//MODE <target> [<modestring> [<mode arguments>...]]
void Server::commandMode(std::vector<std::string> token, Client *user, int fd) {
	if (token.size() == 2) {
		Channel *channel = searchChannel(token[1]);
		if (channel == NULL) {
			sendMessage(ERR_NOSUCHCHANNEL(user->getPrefix(), token[1]), fd);
			return ;
		}
		std::vector<std::string> mode_params = channel->getChannelModeParams();
		sendMessage(RPL_CHANNELMODEIS(user->getPrefix(), channel->getChannelName(), mode_params[0], mode_params[1]), fd);
	}
	else if (token.size() > 2) { //channel mode
		if (token[1][0] == '#') {
			Channel *channel = searchChannel(token[1]);
			if (channel == NULL) {
				sendMessage(ERR_NOSUCHCHANNEL(user->getPrefix(), token[1]), fd);
				return ;
			}
			try {
				std::vector<std::string> mode_params = channel->setChannelMode(token, user);
				if (mode_params.empty() == false) {
					broadcastChannelMessage(RPL_MODE(user->getPrefix(), channel->getChannelName(), mode_params[0], mode_params[1]), channel);
				}
			} catch (std::exception &e) {
				sendMessage(e.what(), fd);
			}
		}
	}
}

//PART(0) #ch(1) msg(2~)
void	Server::commandPart(std::vector<std::string> token, Client * user, int fd) {
	if (token.size() == 1){
		sendMessage(ERR_NEEDMOREPARAMS(user->getNickname(), token[0]), fd);
		return ;

	}
	Channel * ch = searchChannel(token[1]);
	if (!ch){
		sendMessage(ERR_NOSUCHCHANNEL(user->getNickname(), token[1]), fd);
		return ;
	}
	std::string msg = "";
	if (token.size() > 2)
		msg = getTotalMessage(2, token);
	if (ch->isChannelUser(user) == false){
		sendMessage(ERR_NOTONCHANNEL(user->getNickname(), token[1]), fd);
		return ;
	}
	ch->removeChannelUser(user);
	sendMessage(RPL_PART(user->getPrefix(), token[1]), fd);
	broadcastChannelMessage(RPL_PART(user->getPrefix(), token[1]), ch);
	// std::cout <<"###" << RPL_PART(user->getPrefix(), token[1]) << std::endl;
}


void	Server::commandPrivmsg(std::vector<std::string> token, Client * user, int fd){
	if (token.size() != 3 ){
		sendMessage(ERR_NEEDMOREPARAMS(user->getNickname(), token[0]), fd);
		std::cout << "privmsg error from " << user->getNickname() << std::endl;
		return;
	}
	Channel *ch = searchChannel(token[1]);
	if (!ch)
		sendMessage(ERR_NOSUCHCHANNEL(user->getNickname(), token[1]), fd);
	if (ch->isChannelUser(user) == false){
		sendMessage(ERR_NOTONCHANNEL(user->getPrefix(), user->getNickname()), fd);
	}
	broadcastChannelMessage(RPL_PRIVMSG(user->getPrefix(), token[1], token[2]), ch, fd);
}

void	Server::commandInvite(std::vector<std::string> token, Client * user, int fd){
	// invite nickname #channel - 파라미터
	if (token.size() != 3 ){
		sendMessage(ERR_NEEDMOREPARAMS(user->getNickname(), token[0]), fd);
		return;
	}
	Channel	*ch = searchChannel(token[2]);
	Client	*new_user = searchClient(token[1]);
	// 없는 채널일 경우
	if (!ch){
		sendMessage(ERR_NOSUCHCHANNEL(user->getNickname(), token[2]), fd);
	}
	// 채널에 없는 유저가 보낸 경우
	else if (ch->isChannelUser(user) == false){
		sendMessage(ERR_NOTONCHANNEL(user->getNickname(), token[2]), fd);
	}
	// 오퍼레이터가 아닌 경우
	else if (ch->isChannelOperator(user) == false){
		sendMessage(ERR_CHANOPRIVSNEEDED(user->getNickname(), token[2]), fd);
	}
	// 닉네임이 없는 경우
	else if (!user){
		sendMessage(ERR_NOSUCHNICK(user->getNickname(), token[1]), fd);
	}
	// 이미 채널에 있는 유저일경우
	else if (ch->isChannelUser(new_user) == true){
		sendMessage(ERR_USERONCHANNEL(user->getNickname(), token[1], token[2]), fd);
	}
	// 이미 초대 목록에 있을 경우 - 커맨드 없음
	else if (ch->isInvitedUser(new_user) == true){
		std::cout << "already invited" <<std::endl;
	}
	// 정상실행 ::	해당 유저 인바이트 리스트에 추가
	else{
		ch->addInvitedUser(new_user);
		sendMessage(RPL_INVITE(user->getPrefix(), token[1], token[2]), fd);
		sendMessage(RPL_INVITING(user->getPrefix(), token[1], token[2]), fd);
	}
}

//kick #channel nickname - 파라미터
void	Server::commandKick(std::vector<std::string> token, Client * user, int fd){
	int tokensize = token.size();
	if (tokensize <= 2 ){
		sendMessage(ERR_NEEDMOREPARAMS(user->getNickname(), token[0]), fd);
		return ;
	}
	Channel *ch = searchChannel(token[1]);
	Client	*kickUser = searchClient(token[2]);
	std::string msg = getTotalMessage(3, token);
	if (!ch) {
		sendMessage(ERR_NOSUCHCHANNEL(user->getNickname(), token[1]), fd);
		return ;
	}
	// 사용자가 채널에 없음
	else if (ch->isChannelUser(user) == false){
		sendMessage(ERR_NOTONCHANNEL(user->getNickname(), ch->getChannelName()), fd);
	}
	// 킥할 유저가 서버에 없음
	else if (kickUser == NULL){
		sendMessage(ERR_NOSUCHNICK(user->getNickname(), token[2]), fd);
	}
	// 킥할 유저가 채널에 없음
	else if (ch->isChannelUser(kickUser) == false){
		sendMessage(ERR_USERNOTINCHANNEL(user->getNickname(), token[2], ch->getChannelName()), fd);
	}
	// 권한 없음 :: 사용자가 오퍼레이터가 아니고 오퍼레이터가 존재함
	else if (ch->isChannelOperator(user) == false){
		if (ch->getChannelOperator().size() == 0)
			sendMessage(ERR_CHANOPRIVSNEEDED2(user->getNickname(), ch->getChannelName()), fd);
		else
			sendMessage(ERR_CHANOPRIVSNEEDED(user->getNickname(), ch->getChannelName()), fd);
	}
	//정상실행
	else {
		broadcastChannelMessage(RPL_KICK(user->getPrefix(), ch->getChannelName(), kickUser->getNickname(), msg), ch);
		if (ch->isChannelOperator(kickUser) == true)
			ch->removeChannelOperator(kickUser);
		ch->removeChannelUser(kickUser);
		if (ch->getUserCount() == 0){
			removeChannelList(ch->getChannelName());
		}
	}
}

//TOPIC #channel <new topic params>...
void	Server::commandTopic(std::vector<std::string> token, Client * user, int fd) {
	int tokensize = token.size();
	if (tokensize == 1){
		sendMessage(ERR_NEEDMOREPARAMS(user->getNickname(), token[0]), fd);
		return ;
	}
	Channel * ch = searchChannel(token[1]);
	if (!ch) {
		sendMessage(ERR_NOSUCHCHANNEL(user->getNickname(), token[1]), fd);
		return ;
	}
	if (tokensize == 2) { //topic : view
		std::string topic = ch->getChannelTopic();
		if (topic == "" )
			sendMessage(RPL_NOTOPIC(user->getPrefix(), token[1]), fd);
		else {
			sendMessage(RPL_TOPIC(user->getPrefix(), token[1], topic),fd);
		}
	}
	else if (tokensize >= 3) { //set topic
		std::string topic = getTotalMessage(2, token);
		if ((ch->checkChannelMode('t') == true) && (ch->isChannelOperator(user) == false)){
			sendMessage(ERR_CHANOPRIVSNEEDED(user->getNickname(), token[1]), fd);
			return;
		}
		else if (ch->isChannelUser(user) == false){
			sendMessage(ERR_NOTONCHANNEL(user->getNickname(), ch->getChannelName()), fd);
			return ;
		}
		else {
				std::cout << topic << "::is topic" <<std::endl;
				ch->setChannelTopic(topic);
				broadcastChannelMessage(RPL_MY_TOPIC(user->getPrefix(), ch->getChannelName(), ch->getChannelTopic()), ch);
		}
	}
}

//에러 어떤 케이스인지 모르겠음...
void	Server::commandQuit(std::vector<std::string> token, Client * user, int fd){
	std::string msg = getTotalMessage(1, token);
	Channel	*ch;
	for (std::map<std::string, Channel *>::iterator iter = _channel_list.begin(); \
		iter != _channel_list.end(); iter++) {
			ch = iter->second;
			if (ch->isChannelUser(user) == true) {
				broadcastChannelMessage(RPL_QUIT(user->getPrefix(), msg), ch, fd);
				sendMessage(RPL_PART(user->getPrefix(), ch->getChannelName()), fd);
				if(ch->isChannelOperator(user) == true) {
					ch->removeChannelOperator(user);
				}
				ch->removeChannelUser(user);
			}
		}
	// sendMessage(ERR_QUIT(user->getPrefix(), msg), fd);
	disconnectClient(fd, this->_clients);
}
