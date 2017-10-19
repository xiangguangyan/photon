#ifndef _EVENT_HANDLER_HPP_
#define _EVENT_HANDLER_HPP_

#include <iostream>

namespace photon
{
	class Packet;
	class Connection;
	class Acceptor;
}

namespace photon
{
	class EventHandler
	{
	public:
		virtual void handlePacket(Packet* packet, Connection* connection)
		{
			std::cout << ((DefaultPacket*)packet)->m_message << std::endl;
		}
		virtual void handleAccepted(Connection* connection)
		{
			std::cout << "Accepted" << std::endl;
		}
		virtual void handleConnectionError(Connection* connection)
		{
			std::cout << "Connection error" << std::endl;
		}
		virtual void handleAcceptorError(Acceptor* acceptor)
		{
			std::cout << "Acceptor error" << std::endl;
		}
	};
}

#endif // _EVENT_HANDLER_HPP_
