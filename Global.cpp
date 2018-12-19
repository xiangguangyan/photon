#include<stdio.h>
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
        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sa.sa_flags = 0;
        if ((sigemptyset(&sa.sa_mask) == -1) || sigaction(SIGPIPE, &sa, 0) == -1)
        {
            perror("SIGPIPE");
        }

        sigset_t signal_mask;
        sigemptyset(&signal_mask);
        sigaddset(&signal_mask, SIGPIPE);
        if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) == -1)
        {
            perror("SIGPIPE");
        }
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
