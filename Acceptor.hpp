#ifndef _ACCEPTOR_HPP_
#define _ACCEPTOR_HPP_

#include "IOComponent.hpp"
#include "IOService.hpp"
#include "Connection.hpp"

namespace photon
{
    class PacketResolver;
    class EventHandler;

	class Acceptor : public IOComponent
	{
		friend class IOService;
		friend class IOCPService;

	public:
		Acceptor(IOService* ioService, PacketResolver* packetResolver, EventHandler* eventHandler, int addressFamily, int type, int protocol = 0) :
			IOComponent(ioService, addressFamily, type, protocol),
			m_packetResolver(packetResolver),
			m_eventHandler(eventHandler),
			m_addressFamily(addressFamily),
			m_type(type),
			m_protocol(protocol)
		{

		}

		bool listen(const sockaddr* addr, int addrLen)
		{
			return 	m_socket.bind(addr, addrLen) && m_socket.listen() && m_ioService->addAcceptor(this) && m_ioService->startAccept(this);
		}

        Connection* accept()
        {
            Socket socket = m_socket.accept();
            if (socket)
            {
                return new Connection(m_ioService, m_packetResolver, m_eventHandler, std::move(socket));
            }
            return nullptr;
        }

	private:
		virtual bool handleReadComplete()
		{
			return false;
		}
		virtual bool handleWriteComplete()
		{
			return false;
		}
		virtual bool handleAcceptComplete()
		{
			return true;
		}
		virtual bool handleConnectComplete()
		{
			return false;
		}
		virtual bool handleIOError()
		{
            m_eventHandler->handleAcceptorError(this);
			return m_ioService->removeAcceptor(this) && close();
		}

		Connection* createConnection()
		{
			return new Connection(m_ioService, m_packetResolver, m_eventHandler, m_addressFamily, m_type, m_protocol);
		}

	protected:
		PacketResolver* m_packetResolver;
		EventHandler* m_eventHandler;
		int m_addressFamily;
		int m_type;
		int m_protocol;
	};
}

#endif // _ACCEPTOR_HPP_
