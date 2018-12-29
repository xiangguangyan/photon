#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
typedef int socklen_t;
typedef int ssize_t;

#elif defined __linux__
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include<sys/ioctl.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#endif

#include <iostream>

namespace photon
{
    class Socket
    {
    public:
        Socket(const Socket& socket) = delete;

        Socket(Socket&& socket) :
            m_socket(socket.m_socket)
        {
            socket.m_socket = INVALID_SOCKET;
        }

        const Socket& operator = (const Socket& socket) = delete;

        const Socket& operator = (Socket&& socket)
        {
            m_socket = socket.m_socket;
            socket.m_socket = INVALID_SOCKET;
            return *this;
        }

        Socket(SOCKET socket = INVALID_SOCKET) :
            m_socket(socket)
        {

        }

        Socket(int addressFamily, int type, int protocol = 0) :
            m_socket(INVALID_SOCKET)
        {
            init(addressFamily, type, protocol);
        }

        ~Socket()
        {
            close();
        }

        bool init(int addressFamily, int type, int protocol = 0)
        {
            if (INVALID_SOCKET != m_socket)
            {
                return true;
            }

            m_socket = ::socket(addressFamily, type, protocol);

            if (INVALID_SOCKET == m_socket)
            {
                std::cout << "Init socket failed:" << error() << std::endl;
                return false;
            }
            return true;
        }

        bool bind(const sockaddr* addr, socklen_t addrLen)
        {
            if (0 != ::bind(m_socket, addr, addrLen))
            {
                std::cout << "Socket bind failed:" << error() << std::endl;
                return false;
            }

            return true;
        }

        bool bind(uint16_t port, const char* ipaddr = nullptr, int addressFamily = AF_INET)
        {
            if (addressFamily == AF_INET)
            {
                sockaddr_in addr{ 0 };
                addr.sin_family = AF_INET;
                if (ipaddr == nullptr)
                {
                    addr.sin_addr.s_addr = INADDR_ANY;
                }
                else
                {
                    if (::inet_pton(AF_INET, ipaddr, &addr.sin_addr) <= 0)
                    {
                        std::cout << "Socket inet pton failed" << std::endl;
                        return false;
                    }
                }
                addr.sin_port = htons(port);
                return bind((const sockaddr*)&addr, sizeof(addr));
            }
            else if (addressFamily == AF_INET6)
            {
                sockaddr_in6 addr{ 0 };
                addr.sin6_family = AF_INET6;
                if (ipaddr == nullptr)
                {
                    addr.sin6_addr = in6addr_any;
                }
                else
                {
                    if (::inet_pton(AF_INET6, ipaddr, &addr.sin6_addr) <= 0)
                    {
                        std::cout << "Socket inet pton failed" << std::endl;
                        return false;
                    }
                }
                addr.sin6_port = htons(port);
                return bind((const sockaddr*)&addr, sizeof(addr));
            }
            else
            {
                return false;
            }
        }

        bool listen()
        {
            if (0 != ::listen(m_socket, SOMAXCONN))
            {
                std::cout << "Socket listen failed:" << error() << std::endl;
                return false;
            }

            return true;
        }

        Socket accept(sockaddr* addr = nullptr, socklen_t* addrLen = nullptr)
        {
            return ::accept(m_socket, addr, addrLen);
        }

        bool connect(const sockaddr* addr, int addrLen)
        {
            if (0 != ::connect(m_socket, addr, addrLen))
            {
                std::cout << "Socket connect failed:" << error() << std::endl;
                return false;
            }

            return true;
        }

        bool connect(const char* ipaddr, uint16_t port, int addressFamily = AF_INET)
        {
            if (addressFamily == AF_INET)
            {
                sockaddr_in addr{ 0 };
                addr.sin_family = AF_INET;
                if (::inet_pton(AF_INET, ipaddr, &addr.sin_addr) <= 0)
                {
                    std::cout << "Socket inet pton failed" << std::endl;
                    return false;
                }
                addr.sin_port = htons(port);
                return connect((const sockaddr*)&addr, sizeof(addr));
            }
            else if (addressFamily == AF_INET6)
            {
                sockaddr_in6 addr{ 0 };
                addr.sin6_family = AF_INET6;
                if (::inet_pton(AF_INET6, ipaddr, &addr.sin6_addr) <= 0)
                {
                    std::cout << "Socket inet pton failed" << std::endl;
                    return false;
                }
                addr.sin6_port = htons(port);
                return connect((const sockaddr*)&addr, sizeof(addr));
            }
            else
            {
                return false;
            }
        }

        bool setOption(int level, int name, const void* value, socklen_t valueLen)
        {
#ifdef _WIN32
            if (0 != ::setsockopt(m_socket, level, name, (const char*)value, valueLen))
#elif defined __linux__
            if (0 != ::setsockopt(m_socket, level, name, value, valueLen))
#endif
            {
                std::cout << "Socket set option failed:" << error() << std::endl;
                return false;
            }

            return true;
        }

        bool getOption(int level, int name, void* value, socklen_t& valueLen) const
        {
#ifdef _WIN32
            if (0 != ::getsockopt(m_socket, level, name, (char*)value, &valueLen))
#elif defined __linux__
            if (0 != ::getsockopt(m_socket, level, name, value, &valueLen))
#endif
            {
                std::cout << "Socket get option failed:" << error() << std::endl;
                return false;
            }

            return true;
        }

		bool setKeepAlive(bool keepAlive, int idle = 60, int interval = 3, int count = 10)
		{
			if (!keepAlive)
			{
				int keepAliveValue = 0;
				return setOption(SOL_SOCKET, SO_KEEPALIVE, &keepAliveValue, sizeof(keepAliveValue));
			}

			int keepAliveValue = 1;

			return setOption(SOL_SOCKET, SO_KEEPALIVE, &keepAliveValue, sizeof(keepAliveValue)) &&
				setOption(IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle)) &&
				setOption(IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval)) &&
				setOption(IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
		}

        bool setSendBuffer(int size)
        {
            return setOption(SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
        }

        bool setRecvBuffer(int size)
        {
            return setOption(SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
        }

        bool setReuseAddr(bool reuse)
        {
            int value = reuse ? 1 : 0;
            return setOption(SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
        }

        bool setReusePort(bool reuse)
        {
            int value = reuse ? 1 : 0;
#ifdef _WIN32
            return setOption(SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
#else
            return setOption(SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value));
#endif
        }

        bool setBlock(bool block)
        {
            unsigned long mode = block ? 0 : 1;
#ifdef _WIN32
            if (0 != ::ioctlsocket(m_socket, FIONBIO, &mode))
#elif defined __linux__
            if (0 != ::ioctl(m_socket, FIONBIO, &mode))
#endif
            {
                std::cout << "Socket ioctlsocket set block failed:" << error() << std::endl;
                return false;
            }

            return true;
        }

        long dataSize()
        {
            unsigned long size = 0;
#ifdef _WIN32
            if (0 != ::ioctlsocket(m_socket, FIONREAD, &size))
#elif defined __linux__
            if (0 != ::ioctl(m_socket, FIONREAD, &size))
#endif
            {
                std::cout << "Socket ioctlsocket get data size failed:" << error() << std::endl;
                return -1;
            }

            return size;
        }

        ssize_t read(char* buffer, int length)
        {
            ssize_t ret = ::recv(m_socket, buffer, length, 0);
            if (ret >= 0)
            {
                return ret;
            }

#ifdef _WIN32
            if (WSAEWOULDBLOCK == error())
#elif defined __linux__
            if (EAGAIN == error())
#endif
            {
                return -2;
            }

            return -1;
        }

        ssize_t write(const char* buffer, int length)
        {
            ssize_t ret = ::send(m_socket, buffer, length, 0);
            if (ret >= 0)
            {
                return ret;
            }

#ifdef _WIN32
            if (WSAEWOULDBLOCK == error())
#elif defined __linux__
            if (EAGAIN == error())
#endif
            {
                return -2;
            }

            return -1;
        }

        bool close()
        {
            if (INVALID_SOCKET == m_socket)
            {
                return true;
            }

#ifdef _WIN32
            if (0 != ::closesocket(m_socket))
#elif defined __linux__
            if (0 != ::close(m_socket))
#endif
            {
                std::cout << "Socket close failed:" << error() << std::endl;
                return false;
            }

            m_socket = INVALID_SOCKET;
            return true;
        }

        SOCKET getSocket() const
        {
            return m_socket;
        }

        bool getSocketName(sockaddr* addr, socklen_t& addrLen) const
        {
            if (0 != ::getsockname(m_socket, addr, &addrLen))
            {
                std::cout << "Socket get socket name failed:" << error() << std::endl;
                return false;
            }

            return true;
        }

        bool getPeerName(sockaddr* addr, socklen_t& addrLen) const
        {
            if (0 != ::getpeername(m_socket, addr, &addrLen))
            {
                std::cout << "Socket get peer name failed:" << error() << std::endl;
                return false;
            }

            return true;
        }

        operator bool() const
        {
            return INVALID_SOCKET != m_socket;
        }

        inline int error() const
        {
#ifdef _WIN32
            return ::WSAGetLastError();
#elif defined __linux__
            return errno;
#endif
        }

    private:
        SOCKET m_socket;
	};
}

#endif //_SOCKET_HPP_