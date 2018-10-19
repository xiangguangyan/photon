#ifndef _IO_SERVICE_HPP_
#define _IO_SERVICE_HPP_

#include "IOComponent.hpp"

namespace photon
{
	class Connection;
	class Acceptor;
}

namespace photon
{
	class IOCompleteEvent
	{
	public:
		enum IOCompleteType
		{
			READ_COMPLETE,
			WRITE_COMPLETE,
			ACCEPT_COMPLETE,
			CONNECT_COMPLETE,
			IO_ERROR
		};

	public:
		IOCompleteType m_type;
		IOComponent* m_component;
	};

	class IOService
	{
	public:
		IOService() :
			m_running(false)
		{

		}

		void stop()
		{
			m_running = false;
		}

		virtual bool addConnection(Connection* connection) = 0;
		virtual bool addAcceptor(Acceptor* acceptor) = 0;
		virtual bool removeConnection(Connection* connection) = 0;
		virtual bool removeAcceptor(Acceptor* acceptor) = 0;

		virtual bool startRead(Connection* connection) = 0;
		virtual bool startWrite(Connection* connection) = 0;
		virtual bool startAccept(Acceptor* acceptor) = 0;
		virtual bool startConnect(Connection* connection, const sockaddr* addr, int addrLen, const sockaddr* localAddr = nullptr) = 0;

		static const unsigned int MAX_COMPLETE_COUNT = 1024;
		virtual int getCompleteIOComponent(IOCompleteEvent(&completeEvents)[MAX_COMPLETE_COUNT], int32_t timeOutMilliSeconds) = 0;

		void run()
		{
			IOCompleteEvent completeEvents[MAX_COMPLETE_COUNT];
			int size = 0;

			m_running = true;
			while (m_running)
			{
				size = getCompleteIOComponent(completeEvents, -1);

				if (size > 0)
				{
					for (int i = 0; i < size; ++i)
					{
						switch (completeEvents[i].m_type)
						{
						case IOCompleteEvent::READ_COMPLETE:
							completeEvents[i].m_component->handleReadComplete();
							break;
						case IOCompleteEvent::WRITE_COMPLETE:
							completeEvents[i].m_component->handleWriteComplete();
							break;
						case IOCompleteEvent::ACCEPT_COMPLETE:
							completeEvents[i].m_component->handleAcceptComplete();
							break;
						case IOCompleteEvent::CONNECT_COMPLETE:
							completeEvents[i].m_component->handleConnectComplete();
							break;
						case IOCompleteEvent::IO_ERROR:
							completeEvents[i].m_component->handleIOError();
							break;
						default:
							std::cout << "Invalid io complete event type!" << std::endl;
							break;
						}
					}
				}
				else if (size == 0)
				{
					//³¬Ê±
					std::cout << "IOService get complete io component time out!" << std::endl;
				}
				else
				{
					std::cout << "IOService get complete io component failed!" << std::endl;
				}
			}
		}

	private:
		std::atomic<bool> m_running;
	};
}

#endif // _IO_SERVICE_HPP_
