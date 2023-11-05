#ifndef UTIL_H
# define UTIL_H

# include <fcntl.h>
# include <arpa/inet.h>
# include <sys/event.h>
# include <sys/time.h>
# include <unistd.h>
# include <vector>
# include <map>
# include <set>
# include <iostream>
# include <sstream>

# include "message.h"

// for class client
# define CLIENT_CHANNEL_LIMIT 10
# define D_REALNAME "default_realname"


// for class channel
# define CHANNEL_USER_LIMIT 1000



#endif
