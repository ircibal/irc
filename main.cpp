#include "util.h"
#include "Server.hpp"

int main(int argc, char **argv) {
	Server server;
	if (argc != 3) {
		std::cout << "argument error : port" << "pass" << std::endl;
		return 1;
	}
	if (atoi(argv[1]) == 0)
		std::cout << "did you really input 0? well.. I'll just look at it this time" << std::endl;
	if (atoi(argv[1]) > 65535 || atoi(argv[1]) < 1) {
		std::cout << "but i can't watch this" << std::endl;
		return 1;
	}
	server.serverInit(argv);
	return 0;
}
