#ifndef _CONNECTION_HPP_
#define _CONNECTION_HPP_

#include "IOComponent.hpp"
#include "Packet.hpp"
#include "BufferQueue.hpp"
#include "PacketResolver.hpp"
#include "EventHandler.hpp"
#include "Queue.hpp"

namespace photon
{
	class Connection : public IOComponent
	{
		friend class IOService;
		friend class IOCPService;
        friend class EpollService;

	public:
		Connection(IOService* ioService, PacketResolver* packetResolver, EventHandler* eventHandler, int addressFamily, int type, int protocol = 0) :
			IOComponent(ioService, addressFamily, type, protocol),
			m_packetResolver(packetResolver),
			m_eventHandler(eventHandler),
			m_gotHeader(false),
            m_acceptor(nullptr)
		{

		}

        Connection(IOService* ioService, PacketResolver* packetResolver, EventHandler* eventHandler, Socket&& socket) :
            IOComponent(ioService, std::move(socket)),
            m_packetResolver(packetResolver),
            m_eventHandler(eventHandler),
            m_gotHeader(false),
            m_acceptor(nullptr)
        {

        }

        Acceptor* getAcceptor()
        {
            return m_acceptor;
        }

		bool connected()
		{
			return m_state.load() & IOComponent::CONNECTED;
		}

		bool connect(const sockaddr* addr, socklen_t addrLen, bool asyn = false, const sockaddr* localAddr = nullptr)
		{
			if (asyn)
			{
				return m_ioService->addConnection(this) && m_ioService->startConnect(this, addr, addrLen, localAddr);
			}

			if (nullptr != localAddr)
			{
				if (!m_socket.bind(localAddr, addrLen))
				{
					return false;
				}
			}

            m_socket.setBlock(true);
			if (!m_socket.connect(addr, addrLen))
			{
				return false;
			}

			m_state |= IOComponent::CONNECTED;
			return m_ioService->addConnection(this) && m_ioService->startRead(this);
		}

        bool connect(const char* ipaddr, uint16_t port, bool asyn = false, int addressFamily = AF_INET, const char* localip = nullptr, uint16_t localport = 0)
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

                if (localip == nullptr)
                {
                    return connect((const sockaddr*)&addr, sizeof(addr), asyn);
                }
                else
                {
                    sockaddr_in localaddr{ 0 };
                    localaddr.sin_family = AF_INET;
                    if (::inet_pton(AF_INET, localip, &localaddr.sin_addr) <= 0)
                    {
                        std::cout << "Socket inet pton failed" << std::endl;
                        return false;
                    }
                    localaddr.sin_port = htons(localport);

                    return connect((const sockaddr*)&addr, sizeof(addr), asyn, (const sockaddr*)&localaddr);
                }
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

                if (localip == nullptr)
                {
                    return connect((const sockaddr*)&addr, sizeof(addr), asyn);
                }
                else
                {
                    sockaddr_in6 localaddr{ 0 };
                    localaddr.sin6_family = AF_INET6;
                    if (::inet_pton(AF_INET6, localip, &localaddr.sin6_addr) <= 0)
                    {
                        std::cout << "Socket inet pton failed" << std::endl;
                        return false;
                    }
                    localaddr.sin6_port = htons(localport);

                    return connect((const sockaddr*)&addr, sizeof(addr), asyn, (const sockaddr*)&localaddr);
                }
            }
            else
            {
                return false;
            }
        }

        bool read()
        {
            while (Buffer buffer = m_readQueue.reserve())
            {
                ssize_t ret = m_socket.read(buffer.getData(), buffer.getSize());
                if (ret > 0)
                {
                    m_readQueue.push((uint32_t)ret);
                    if ((uint32_t)ret < buffer.getSize())
                    {
                        return true;
                    }
                }
                else if (ret == -2)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
           
            return true;
        }

        bool write()
        {
            while (const Buffer buffer = m_writeQueue.front())
            {
                ssize_t ret = m_socket.write(buffer.getData(), buffer.getSize());
                if (ret > 0)
                {
                    m_writeQueue.pop((uint32_t)ret);
                    if ((uint32_t)ret < buffer.getSize())
                    {
                        return true;
                    }
                }
                else if (ret == -2)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }

            return true;
        }

		bool postPacket(const Packet* packet)
		{
            m_writeQueue.lock();

            bool ret = m_packetResolver->serialize(packet, m_writeQueue);

            if (!(m_state.load() & IOComponentState::WRITING) && !m_writeQueue.empty())
            {
                if (!m_ioService->startWrite(this))
                {
                    m_writeQueue.unlock();
                    handleIOError();
                    return false;
                }
            }

            m_writeQueue.unlock();
            return ret;
		}

	private:
		virtual bool handleReadComplete()
		{
			m_readQueue.lock();

			Queue<Packet*, 128> readPackets;

			if (!m_gotHeader)
			{
				//没有获取header
				m_gotHeader = m_packetResolver->getPacketHeader(m_header, m_readQueue);
				if (!m_gotHeader)
				{
					if (!(m_state.load() & IOComponent::READING))
					{
                        if (!m_ioService->startRead(this))
                        {
                            m_readQueue.unlock();
                            handleIOError();
                            return false;
                        }
					}

					m_readQueue.unlock();
					return false;
				}
			}

			while (m_gotHeader && !readPackets.full())
			{
				Packet* packet = m_packetResolver->deserialize(m_header, m_readQueue);
				if (packet == nullptr)
				{
					break;
				}

				readPackets.push(packet);
				m_gotHeader = m_packetResolver->getPacketHeader(m_header, m_readQueue);
			}

			if (!(m_state.load() & IOComponent::READING))
			{
                if (!m_ioService->startRead(this))
                {
                    m_readQueue.unlock();
                    handleIOError();
                    return false;
                }
			}

			m_readQueue.unlock();
          

			Packet* packet = nullptr;
			while (!readPackets.empty())
			{
				readPackets.pop(packet);
                m_eventHandler->handlePacket(packet, this);
				m_packetResolver->freePacket(packet);
			}

			return true;
		}

		virtual bool handleWriteComplete()
		{
			return true;
		}

        virtual bool handleAcceptComplete()
        {
			m_readQueue.lock();
			if (!m_ioService->addConnection(this))
			{
				m_readQueue.unlock();
				decRef();
				return false;
			}
			if (!m_ioService->startRead(this))
			{
				m_readQueue.unlock();
				handleIOError();
				return false;
			}
			m_readQueue.unlock();
			m_eventHandler->handleAccepted(this);
			return true;
        }

		virtual bool handleConnectComplete()
		{
			m_readQueue.lock();
            if (!m_ioService->startRead(this))
            {
                m_readQueue.unlock();
                handleIOError();
                return false;
            }
			m_readQueue.unlock();
            m_eventHandler->handleConnected(this);
			return true;
		}

		virtual bool handleIOError()
		{
            if (m_ioService->removeConnection(this)/* && m_socket.close() */)
            {
                m_eventHandler->handleConnectionError(this);
                if (m_acceptor != nullptr)
                {
                    decRef();
                }
                return true;
            }
            return false;
		}

	protected:
		ReadBufferQueue m_readQueue;
		WriteBufferQueue m_writeQueue;
		PacketResolver* m_packetResolver;
		EventHandler* m_eventHandler;
		bool m_gotHeader;
		PacketHeader m_header;
        Acceptor* m_acceptor;
	};
}

#endif //_CONNECTION_HPP_