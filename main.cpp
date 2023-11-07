#include "util.h"
#include "Server.hpp"

int port_atoi(char *str){
	int	res = 0;
	
	while (*str){
		if (res > 65535)
			return (-1);
		if (*str > '9' || *str < '0')
			return (-1);
		res = res * 10 + *str - '0';
		str++;
	}
	return (res);
}

int main(int argc, char **argv) {
	Server server;
	if (argc != 3) {
		std::cout << "argument error : port" << "pass" << std::endl;
		return 1;
	}
	if (atoi(argv[1]) == 0)
		std::cout << "ERROR:: did you really input 0? well.. I'll just look at it this time" << std::endl;
	if (port_atoi(argv[1]) == -1) {
		std::cout << "ERROR:: port number is wrong" << std::endl;
		return 1;
	}
	server.serverInit(argv);
	return 0;
}
