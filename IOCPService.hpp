#ifdef _WIN32

#ifndef _IOCP_SERVICE_HPP_
#define _IOCP_SERVICE_HPP_

#include <mswsock.h>
#include <iostream>

#include "IOService.hpp"
#include "IOComponent.hpp"
#include "Connection.hpp"
#include "Acceptor.hpp"

namespace photon
{
	class IOCPService : public IOService
	{
	private:
		class Overlapped : public OVERLAPPED
		{
		public:
			enum IOOperation
			{
				NONE,
				READ,
				WRITE,
				ACCEPT,
				CONNECT
			};

		public:
			Overlapped() :
				OVERLAPPED({ 0 }),
				m_operation(IOOperation::NONE)
			{

			}

			IOOperation m_operation;
		};

		class ConnectionOverlapped : public Overlapped
		{
		public:
			ConnectionOverlapped()
			{
				m_buffer.buf = nullptr;
				m_buffer.len = 0;
			}

			WSABUF m_buffer;
		};

		class AcceptorOverlapped : public Overlapped
		{
		public:
			AcceptorOverlapped() :
				m_connection(nullptr)
			{

			}

			Connection* m_connection;
			char m_buffer[64];
		};

		class ConnectionCompletionKey
		{
		public:
			Connection* m_connection;
			ConnectionOverlapped m_readOverlapped;
			ConnectionOverlapped m_writeOverlapped;
		};

		class AcceptorCompletionKey
		{
		public:
			Acceptor* m_acceptor;
			AcceptorOverlapped m_acceptorOverlapped;
		};

	public:
		IOCPService()
		{
			m_iocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, NULL, 0);
			if (nullptr == m_iocp)
			{
				std::cout << "Create io completin port failed:" << ::GetLastError() << std::endl;
			}
		}

        ~IOCPService()
        {
            if (nullptr != m_iocp)
            {
                CloseHandle(m_iocp);
            }
        }

		virtual bool addConnection(Connection* connection) override
		{
			if (nullptr != connection->m_userData)
			{
				//事先已经add，不能多次add
				return false;
			}

			ConnectionCompletionKey* completionKey = new ConnectionCompletionKey();
			completionKey->m_connection = connection;

			if (nullptr == ::CreateIoCompletionPort((HANDLE)connection->getSocket().getSocket(), m_iocp, (ULONG_PTR)completionKey, 0))
			{
				std::cout << "Add connection create io completion port failed:" << ::GetLastError() << std::endl;
				delete completionKey;
				return false;
			}

            connection->addRef();
			connection->m_userData = completionKey;
            connection->getSocket().setBlock(false);
			return true;
		}

		virtual bool addAcceptor(Acceptor* acceptor) override
		{
			if (nullptr != acceptor->m_userData)
			{
				//事先已经add，不能多次add
				return false;
			}

			AcceptorCompletionKey* completionKey = new AcceptorCompletionKey();
			completionKey->m_acceptor = acceptor;

			if (nullptr == ::CreateIoCompletionPort((HANDLE)acceptor->getSocket().getSocket(), m_iocp, (ULONG_PTR)completionKey, 0))
			{
				std::cout << "Add acceptor create io completion port failed:" << ::GetLastError() << std::endl;
				delete completionKey;
				return false;
			}

            acceptor->addRef();
			acceptor->m_userData = completionKey;
            acceptor->getSocket().setBlock(false);
			return true;
		}

		virtual bool removeConnection(Connection* connection) override
		{
			void* data = connection->m_userData;
			if (nullptr == data)
			{
				//事先没有add，不能remove
				return false;
			}

			delete (ConnectionCompletionKey*)data;

			connection->m_userData = nullptr;
            connection->decRef();
			return true;
		}

		virtual bool removeAcceptor(Acceptor* acceptor) override
		{
			void* data = acceptor->m_userData;
			if (nullptr == data)
			{
				//事先没有add，不能remove
				return false;
			}

			delete (AcceptorCompletionKey*)data;

			acceptor->m_userData = nullptr;
            acceptor->decRef();
			return true;
		}

		virtual bool startRead(Connection* connection) override
		{
			if (connection->m_state.load() & IOComponent::IOComponentState::READING)
			{
                return true;
			}

			void* data = connection->m_userData;
			if (nullptr == data)
			{
				//事先没有add	
				return false;
			}

            Buffer buffer = connection->m_readQueue.reserve();
            if (!buffer)
            {
                return true;
            }

			ConnectionOverlapped* overlapped = &((ConnectionCompletionKey*)data)->m_readOverlapped;
			overlapped->m_operation = Overlapped::READ;
			overlapped->m_buffer.buf = buffer.getData();
			overlapped->m_buffer.len = buffer.getSize();

			DWORD numberOfBytesRecv = 0;
			DWORD flags = 0;
			int ret = ::WSARecv(connection->getSocket().getSocket(), &overlapped->m_buffer, 1, &numberOfBytesRecv, &flags, overlapped, nullptr);

			if (0 == ret || WSA_IO_PENDING == ::WSAGetLastError())
			{
				connection->m_state |= IOComponent::IOComponentState::READING;
				return true;
			}
			
			return false;
		}

		virtual bool startWrite(Connection* connection) override
		{
			if (connection->m_state.load() & IOComponent::IOComponentState::WRITING)
			{
                return true;
			}

			void* data = connection->m_userData;
			if (nullptr == data)
			{
				//事先没有add
				return false;
			}

            Buffer buffer = connection->m_writeQueue.front();
            if (!buffer)
            {
                return true;
            }

			ConnectionOverlapped* overlapped = &((ConnectionCompletionKey*)data)->m_writeOverlapped;
			overlapped->m_operation = Overlapped::WRITE;
            overlapped->m_buffer.buf = buffer.getData();
            overlapped->m_buffer.len = buffer.getSize();

			DWORD numberOfBytesSend = 0;
			int ret = ::WSASend(connection->getSocket().getSocket(), &overlapped->m_buffer, 1, &numberOfBytesSend, 0, overlapped, nullptr);

			if (0 == ret || WSA_IO_PENDING == ::WSAGetLastError())
			{
				connection->m_state |= IOComponent::IOComponentState::WRITING;
				return true;
			}
			
			return false;
		}

		LPFN_ACCEPTEX getAcceptExFunction(SOCKET socket)
		{
			GUID acceptExGuid = WSAID_ACCEPTEX;
			LPFN_ACCEPTEX acceptExFunction = nullptr;
			DWORD bytes = 0;
			WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptExGuid, sizeof(acceptExGuid), &acceptExFunction, sizeof(acceptExFunction), &bytes, NULL, NULL);
			return acceptExFunction;
		}

		virtual bool startAccept(Acceptor* acceptor) override
		{
			if (acceptor->m_state.load() & IOComponent::ACCEPTING)
			{
                return true;
			}

			void* data = acceptor->m_userData;
			if (nullptr == data)
			{
				//事先没有add
				return false;
			}

			AcceptorOverlapped* overlapped = &((AcceptorCompletionKey*)data)->m_acceptorOverlapped;
			overlapped->m_connection = acceptor->createConnection();
			if (overlapped->m_connection == nullptr)
			{
				return false;
			}

			overlapped->m_operation = Overlapped::ACCEPT;

			static LPFN_ACCEPTEX AcceptExFunction = getAcceptExFunction(acceptor->getSocket().getSocket());

			BOOL ret = AcceptExFunction(acceptor->getSocket().getSocket(), overlapped->m_connection->getSocket().getSocket(), overlapped->m_buffer, 0, sizeof(overlapped->m_buffer) / 2, sizeof(overlapped->m_buffer) / 2, nullptr, overlapped);

			if (ret || WSA_IO_PENDING == ::WSAGetLastError())
			{
				acceptor->m_state |= IOComponent::IOComponentState::ACCEPTING;
				return true;
			}
			
			return false;
		}

		LPFN_CONNECTEX getConnectExFunction(SOCKET socket)
		{
			GUID connectExGuid = WSAID_CONNECTEX;
			LPFN_CONNECTEX connectExFunction = nullptr;
			DWORD bytes = 0;
			WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &connectExGuid, sizeof(connectExGuid), &connectExFunction, sizeof(connectExFunction), &bytes, NULL, NULL);
			return connectExFunction;
		}

		virtual bool startConnect(Connection* connection, const sockaddr* addr, socklen_t addrLen, const sockaddr* localAddr = nullptr) override
		{
			if (connection->m_state.load() & (IOComponent::IOComponentState::CONNECTING | IOComponent::IOComponentState::CONNECTED))
			{
				//已connect，不能再次start
				return false;
			}

			void* data = connection->m_userData;
			if (nullptr == data)
			{
				//事先没有add
				return false;
			}

			//ConnectEx之前必须要先bind
			if (nullptr != localAddr)
			{
				if (!connection->getSocket().bind(localAddr, addrLen))
				{
					return false;
				}
			}
			else
			{
				uint64_t addrBuf[4] = { 0 };
				sockaddr* a = reinterpret_cast<sockaddr*>(addrBuf);
				a->sa_family = addr->sa_family;

				if (!connection->getSocket().bind(a, addrLen))
				{
					return false;
				}
			}

			ConnectionOverlapped* overlapped = &((ConnectionCompletionKey*)data)->m_readOverlapped;

			overlapped->m_operation = Overlapped::CONNECT;

			static LPFN_CONNECTEX ConnectExFunction = getConnectExFunction(connection->getSocket().getSocket());

			BOOL ret = ConnectExFunction(connection->getSocket().getSocket(), addr, addrLen, nullptr, 0, nullptr, overlapped);

			if (ret || WSA_IO_PENDING == ::WSAGetLastError())
			{
				connection->m_state |= IOComponent::IOComponentState::CONNECTING;
				return true;
			}
			
			return false;
		}

		virtual int getCompleteIOComponent(IOCompleteEvent(&completeEvents)[MAX_SOCKET_COUNT], int32_t timeOutMilliSeconds) override
		{
            OVERLAPPED_ENTRY entries[MAX_SOCKET_COUNT];
            ULONG numEntriesRemoved = 0;
			if (!::GetQueuedCompletionStatusEx(m_iocp, entries, MAX_SOCKET_COUNT, &numEntriesRemoved, timeOutMilliSeconds, TRUE))
			{
                if (WAIT_TIMEOUT == ::GetLastError())
                {
                    return 0;
                }
                return -1;
			}

            int completeCount = numEntriesRemoved;
            for (uint32_t i = 0; i < numEntriesRemoved; ++i)
            {
                Overlapped* overlapped = (Overlapped*)entries[i].lpOverlapped;

                switch (overlapped->m_operation)
                {
                case Overlapped::IOOperation::READ:
                {
                    Connection* connection = ((ConnectionCompletionKey*)entries[i].lpCompletionKey)->m_connection;
                    connection->addRef();
                    connection->m_readQueue.lock();
                    connection->m_state &= ~IOComponent::IOComponentState::READING;

                    completeEvents[i].m_component = connection;

                    if (entries[i].dwNumberOfBytesTransferred == 0)
                    {
                        connection->m_state |= IOComponent::IOComponentState::IOERROR;
                        completeEvents[i].m_type = IOCompleteEvent::IO_ERROR;
                    }
                    else
                    {
                        completeEvents[i].m_type = IOCompleteEvent::READ_COMPLETE;

                        connection->m_readQueue.push(entries[i].dwNumberOfBytesTransferred);
                        if (!connection->m_readQueue.full())
                        {
                            if (!startRead(connection))
                            {
                                connection->m_state |= IOComponent::IOComponentState::IOERROR;
                                completeEvents[i].m_type |= IOCompleteEvent::IO_ERROR;
                            }
                        }
                    }

                    connection->m_readQueue.unlock();

                    break;
                }

                case Overlapped::IOOperation::WRITE:
                {
                    Connection* connection = ((ConnectionCompletionKey*)entries[i].lpCompletionKey)->m_connection;
                    connection->addRef();
                    connection->m_writeQueue.lock();
                    connection->m_state &= ~IOComponent::IOComponentState::WRITING;

                    completeEvents[i].m_component = connection;

                    if (entries[i].dwNumberOfBytesTransferred == 0)
                    {
                        connection->m_state |= IOComponent::IOComponentState::IOERROR;
                        completeEvents[i].m_type = IOCompleteEvent::IO_ERROR;
                    }
                    else
                    {
                        completeEvents[i].m_type = IOCompleteEvent::WRITE_COMPLETE;

                        connection->m_writeQueue.pop(entries[i].dwNumberOfBytesTransferred);
                        if (!connection->m_writeQueue.empty())
                        {
                            if (!startWrite(connection))
                            {
                                connection->m_state |= IOComponent::IOComponentState::IOERROR;
                                completeEvents[i].m_type |= IOCompleteEvent::IO_ERROR;
                            }
                        }
                    }

                    connection->m_writeQueue.unlock();

                    break;
                }

                case Overlapped::IOOperation::ACCEPT:
                {
                    Acceptor* acceptor = ((AcceptorCompletionKey*)entries[i].lpCompletionKey)->m_acceptor;
                    acceptor->addRef();
                    acceptor->m_state &= ~IOComponent::ACCEPTING;

                    completeEvents[i].m_component = acceptor;
                    completeEvents[i].m_type = IOCompleteEvent::ACCEPT_COMPLETE;

                    Connection* connection = ((AcceptorCompletionKey*)entries[i].lpCompletionKey)->m_acceptorOverlapped.m_connection;
                    connection->addRef();
                    SOCKET listenSocket = acceptor->getSocket().getSocket();
                    connection->getSocket().setOption(SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const char*)&listenSocket, sizeof(listenSocket));

                    connection->m_state &= ~IOComponent::IOComponentState::CONNECTING;
                    connection->m_state |= IOComponent::IOComponentState::CONNECTED;
                    connection->m_acceptor = acceptor;

                    completeEvents[completeCount].m_component = connection;
                    completeEvents[completeCount].m_type = IOCompleteEvent::ACCEPT_COMPLETE;
                    ++completeCount;

                    if (!startAccept(acceptor))
                    {
                        acceptor->m_state |= IOComponent::IOComponentState::IOERROR;
                        completeEvents[i].m_type |= IOCompleteEvent::IO_ERROR;
                    }

                    break;
                }

                case Overlapped::IOOperation::CONNECT:
                {
                    Connection* connection = ((ConnectionCompletionKey*)entries[i].lpCompletionKey)->m_connection;
                    connection->addRef();
                    completeEvents[i].m_component = connection;

                    int seconds = 0;
                    int bytes = sizeof(seconds);
                    bool ret = connection->getSocket().getOption(SOL_SOCKET, SO_CONNECT_TIME, &seconds, bytes);

                    if (!ret || seconds == 0xFFFFFFFF)
                    {
                        connection->m_state |= IOComponent::IOComponentState::IOERROR;
                        completeEvents[i].m_type = IOCompleteEvent::IO_ERROR;
                    }
                    else
                    {
                        connection->m_state &= ~IOComponent::IOComponentState::CONNECTING;
                        connection->m_state |= IOComponent::IOComponentState::CONNECTED;
                        completeEvents[i].m_type = IOCompleteEvent::CONNECT_COMPLETE;
                    }

                    break;
                }

                default:
                {
                    completeEvents[i].m_type = 0;
                    completeEvents[i].m_component = nullptr;
                    std::cout << "Invalid io operation:" << overlapped->m_operation << std::endl;
                }
                }
            }

            return completeCount;
		}

	private:
		HANDLE m_iocp;
	};
}

#endif // _IOCP_SERVICE_HPP_

#endif // _WIN32