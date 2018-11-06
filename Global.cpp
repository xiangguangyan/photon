#ifdef _WIN32

#include <Socket.hpp>

struct NetEnv
{
    NetEnv()
    {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }

    ~NetEnv()
    {
        WSACleanup();
    }
};

const NetEnv netenv;

#endif