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

        bool listen(uint16_t port, const char* ipaddr = nullptr, int addressFamily = AF_INET)
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
                return listen((const sockaddr*)&addr, sizeof(addr));
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
                return listen((const sockaddr*)&addr, sizeof(addr));
            }
            else
            {
                return false;
            }
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
            bool ret = m_ioService->removeAcceptor(this) && close();
            m_eventHandler->handleAcceptorError(this);
            return ret;
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
