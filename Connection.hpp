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

	public:
		Connection(IOService* ioService, PacketResolver* packetResolver, EventHandler* eventHandler, int addressFamily, int type, int protocol = 0) :
			IOComponent(ioService, addressFamily, type, protocol),
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

		bool connect(const sockaddr* addr, int addrLen, bool asyn = false, const sockaddr* localAddr = nullptr)
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

			if (!m_socket.connect(addr, addrLen))
			{
				return false;
			}

			m_state |= IOComponent::CONNECTED;
			return m_ioService->addConnection(this) && m_ioService->startRead(this);
		}

        bool read()
        {
            Buffer buffer = m_readQueue.reserve();
            if (!buffer)
            {
                return true;
            }
            int ret = m_socket.read(buffer.getData(), buffer.getSize());
            if (ret <= 0)
            {
                return false;
            }

            m_readQueue.push(ret);
            return true;
        }

        bool write()
        {
            const Buffer buffer = m_writeQueue.front();
            if (!buffer)
            {
                return true;
            }
            int ret = m_socket.write(buffer.getData(), buffer.getSize());
            if (ret <= 0)
            {
                return false;
            }

            m_writeQueue.pop(ret);
            return true;
        }

		bool postPacket(const Packet* packet)
		{
			m_writeQueue.lock();

			if (!m_packetResolver->serialize(packet, m_writeQueue))
			{
				m_writeQueue.unlock();
				return false;
			}

			if (!(m_state.load() & IOComponentState::WRITING))
			{
				if (!m_ioService->startWrite(this))
				{
					m_writeQueue.unlock();
					return false;
				}
			}

			m_writeQueue.unlock();
			return true;
		}

	private:
		virtual bool handleReadComplete()
		{
			m_readQueue.lock();

			Queue<Packet*, 32> readPackets;

			if (!m_gotHeader)
			{
				//没有获取header
				m_gotHeader = m_packetResolver->getPacketHeader(m_header, m_readQueue);
				if (!m_gotHeader)
				{
					if (!(m_state.load() & IOComponent::READING))
					{
						m_ioService->startRead(this);
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
				m_ioService->startRead(this);
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
			m_ioService->addConnection(this);
			bool ret = m_ioService->startRead(this);
			m_readQueue.unlock();
			m_eventHandler->handleAccepted(this);
			return ret;
		}

		virtual bool handleConnectComplete()
		{
			m_readQueue.lock();
			bool ret = m_ioService->startRead(this);
			m_readQueue.unlock();
            m_eventHandler->handleConnected(this);
			return ret;
		}

		virtual bool handleIOError()
		{
            m_eventHandler->handleConnectionError(this);
			return m_ioService->removeConnection(this) && close();
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