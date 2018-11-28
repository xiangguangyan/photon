#ifdef _WIN32
#include <Socket.hpp>
#elif defined __linux__
#include <signal.h>
#endif

struct Environment
{
    Environment()
    {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#elif defined __linux__
        signal(SIGPIPE, SIG_IGN);
#endif
    }

    ~Environment()
    {
#ifdef _WIN32
        WSACleanup();
#elif defined __linux__
        
#endif
    }
};

static const Environment environment;
