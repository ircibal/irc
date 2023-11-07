#include "util.h"
#include "Server.hpp"

int main(int argc, char **argv) {
	Server server;
	if (argc != 3) {
		std::cout << "argument error : port" << "pass" << std::endl;
		return 1;
	}
	if (atoi(argv[1]) > 48000 || atoi(argv[1]) < 1) {
		std::cout << "error: port number range" << std::endl;
		return 1;
	}
	server.serverInit(argv);
	return 0;
}
