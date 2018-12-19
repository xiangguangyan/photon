#ifndef _EVENT_HANDLER_HPP_
#define _EVENT_HANDLER_HPP_

#include <iostream>
#include <atomic>

namespace photon
{
    class Packet;
    class Connection;
    class Acceptor;

	class EventHandler
	{
	public:
        EventHandler() :
            counter(0)
        {

        }

		virtual void handlePacket(Packet* packet, Connection* connection)
		{
            //std::cout << ((DefaultPacket*)packet)->m_message << std::endl;
            static int count = 0;
            static auto begin = std::chrono::system_clock::now();
            if (++count >= 1000000)
            {
                auto now = std::chrono::system_clock::now();
                std::cout << "1000000 packet time: " << std::chrono::duration_cast<std::chrono::milliseconds>(now - begin).count() << " ms" << std::endl;
                count = 0;
                begin = now;
            }
		}

		virtual void handleAccepted(Connection* connection)
		{
			std::cout << "Accepted, connection count:" << ++counter << std::endl;
		}

        virtual void handleConnected(Connection* connection)
        {
            std::cout << "Connected" << std::endl;
        }

		virtual void handleConnectionError(Connection* connection)
		{
			std::cout << "Connection error, connection count:" << --counter << std::endl;
		}

		virtual void handleAcceptorError(Acceptor* acceptor)
		{
			std::cout << "Acceptor error" << std::endl;
		}

    private:
        std::atomic<int> counter;
	};
}

#endif // _EVENT_HANDLER_HPP_
