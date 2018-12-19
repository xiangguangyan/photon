#ifndef _IO_SERVICE_HPP_
#define _IO_SERVICE_HPP_

#include "IOComponent.hpp"

namespace photon
{
    class Connection;
    class Acceptor;

	class IOCompleteEvent
	{
	public:
		enum IOCompleteType
		{
			READ_COMPLETE = 1,
			WRITE_COMPLETE = 2,
			ACCEPT_COMPLETE = 4,
			CONNECT_COMPLETE = 8,
			IO_ERROR = 1024
		};

	public:
		uint32_t m_type;
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
		virtual bool startConnect(Connection* connection, const sockaddr* addr, socklen_t addrLen, const sockaddr* localAddr = nullptr) = 0;

		static const unsigned int MAX_SOCKET_COUNT = 1024;
		virtual int getCompleteIOComponent(IOCompleteEvent(&completeEvents)[MAX_SOCKET_COUNT], int32_t timeOutMilliSeconds) = 0;

		void run()
		{
			IOCompleteEvent completeEvents[MAX_SOCKET_COUNT];
			int size = 0;

			m_running = true;
			while (m_running)
			{
				size = getCompleteIOComponent(completeEvents, -1);

				if (size > 0)
				{
					for (int i = 0; i < size; ++i)
					{
                        if (completeEvents[i].m_type & IOCompleteEvent::READ_COMPLETE)
                        {
                            completeEvents[i].m_component->handleReadComplete();
                        }
                        if (completeEvents[i].m_type & IOCompleteEvent::WRITE_COMPLETE)
                        {
                            completeEvents[i].m_component->handleWriteComplete();
                        }
                        if (completeEvents[i].m_type & IOCompleteEvent::ACCEPT_COMPLETE)
                        {
                            completeEvents[i].m_component->handleAcceptComplete();
                        }
                        if (completeEvents[i].m_type & IOCompleteEvent::CONNECT_COMPLETE)
                        {
                            completeEvents[i].m_component->handleConnectComplete();
                        }
                        if (completeEvents[i].m_type & IOCompleteEvent::IO_ERROR)
                        {
                            completeEvents[i].m_component->handleIOError();
                        }

                        completeEvents[i].m_component->decRef();        //addRef() in getCompleteIOComponent
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
		bool m_running;
	};
}

#endif // _IO_SERVICE_HPP_
