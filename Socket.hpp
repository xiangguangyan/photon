#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#ifdef _WIN32
#include <WinSock2.h>
#elif defined _POSIX
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

        Socket(Socket&& socket):
            m_socket(socket.m_socket)
        {
            
        }
        
		const Socket& operator = (const Socket& socket) = delete;

        const Socket& operator = (Socket&& socket)
        {
            m_socket = socket.m_socket;
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

		bool bind(const sockaddr* addr, int addrLen)
		{
			if (0 != ::bind(m_socket, addr, addrLen))
			{
				std::cout << "Socket bind failed:" << error() << std::endl;
				return false;
			}

			return true;
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

		Socket accept(sockaddr* addr = nullptr, int* addrLen = nullptr)
		{
			SOCKET socket = ::accept(m_socket, addr, addrLen);

			if (INVALID_SOCKET == socket)
			{
				std::cout << "Socket accept failed:" << error() << std::endl;
			}

			return Socket(socket);
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

		bool setOption(int level, int name, const char* value, int valueLen)
		{
			if (0 != ::setsockopt(m_socket, level, name, value, valueLen))
			{
				std::cout << "Socket set option failed:" << error() << std::endl;
				return false;
			}

			return true;
		}

		bool getOption(int level, int name, char* value, int& valueLen) const
		{
			if (0 != ::getsockopt(m_socket, level, name, value, &valueLen))
			{
				std::cout << "Socket get option failed:" << error() << std::endl;
				return false;
			}

			return true;
		}

		bool setKeepAlive(bool keepAlive)
		{
			int value = keepAlive ? 1 : 0;
			return setOption(SOL_SOCKET, SO_KEEPALIVE, (const char*)&value, sizeof(value));
		}

		bool setSendBuffer(int size)
		{
			return setOption(SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size));
		}

		bool setRecvBuffer(int size)
		{
			return setOption(SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size));
		}

		bool setReuseAddr(bool reuse)
		{
			int value = reuse ? 1 : 0;
			return setOption(SOL_SOCKET, SO_REUSEADDR, (const char*)&value, sizeof(value));
		}

		bool setBlock(bool block)
		{
			unsigned long mode = block ? 0 : 1;
#ifdef _WIN32
			if (0 != ::ioctlsocket(m_socket, FIONBIO, &mode))
#elif defined _POSIX
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
#elif defined _POSIX
			if (0 != ::ioctl(m_socket, FIONREAD, &size))
#endif
			{
				std::cout << "Socket ioctlsocket get data size failed:" << error() << std::endl;
				return -1;
			}

			return size;
		}

		int read(char* buffer, int length)
		{
			return ::recv(m_socket, buffer, length, 0);
		}

		int write(const char* buffer, int length)
		{
			return ::send(m_socket, buffer, length, 0);
		}

		bool close()
		{
			if (INVALID_SOCKET == m_socket)
			{
				return true;
			}

#ifdef _WIN32
			if (0 != ::closesocket(m_socket))
#elif defined _POSIX
			if(0 != ::close(m_socket))
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

		bool getSocketName(sockaddr* addr, int& addrLen) const
		{
			if (0 != ::getsockname(m_socket, addr, &addrLen))
			{
				std::cout << "Socket get socket name failed:" << error() << std::endl;
				return false;
			}

			return true;
		}

		bool getPeerName(sockaddr* addr, int& addrLen) const
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

		int error() const
		{
#ifdef _WIN32
			return ::WSAGetLastError();
#elif defined _POSIX
			return errno;
#endif
		}

	private:
		SOCKET m_socket;
	};
}

#endif //_SOCKET_HPP_