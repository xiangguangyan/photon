#ifndef _IO_COMPONENT_HPP_
#define _IO_COMPONENT_HPP_

#include <stdint.h>
#include <atomic>

#include "Socket.hpp"

namespace photon
{
    class IOService;
    class IOCPService;
    class EpollService;

	class IOComponent
	{
		friend class IOService;
		friend class IOCPService;
        friend class EpollService;

	public:

		//iocp在WSARecv时置READING状态位，完成后置READ_COMPLETE状态位，清除READING状态位，若接收队列未满则继续WSARecv，否则置READ_QUEUE_FULL状态位；
		//epoll在read时置READING状态位，EWOULDBLOCK时置READ_COMPLETE状态位，清除READING状态位，若其间接收队列先满，置READ_COMPLETE和READ_QUEUE_FULL状态位；
		//handleReadComplete中在数据提交给用户后，需判断READ_QUEUE_FULL状态位，如被置位，需要调用IOService的startRead

		//iocp在WSASend时置WRITING状态位，完成后置WRITE_COMPLETE状态位，清除WRITING状态位，若发送队列未空则继续WSASend，否则置WRITE_BUFFER_EMPTY状态位；
		//epoll在write时置WRITING状态位，EWOULDBLOCK时置WRITE_COMPLETE状态位，清除WRITING状态位，若其间发送队列先空，置WRITE_COMPLETE和WRITE_QUEUE_EMPTY状态位;
		//postPacket中在接收用户提交的数据后，需判断WRITE_QUEUE_EMPTY状态位，如被置位，需要调用IOService的startWrite

		//IO组件对方关闭或者出错时置IOERROR状态位
        enum IOComponentState : uint32_t
        {
            READING = 1u << 31,
            WRITING = 1u << 30,
            ACCEPTING = 1u << 29,
            CONNECTING = 1u << 28,
            CONNECTED = 1u << 27,
            IOERROR = 1u << 26,
        };

	public:
		IOComponent(IOService* ioService, int addressFamily, int type, int protocol = 0) :
			m_userData(nullptr),
			m_socket(addressFamily, type, protocol),
            m_ioService(ioService),
            m_state(1u)
		{

		}

        IOComponent(IOService* ioService, Socket&& socket) :
            m_userData(nullptr),
            m_socket(std::move(socket)),
            m_ioService(ioService),
            m_state(1u)
        {

        }

        virtual ~IOComponent()
        {

        }

		Socket& getSocket()
		{
			return m_socket;
		}

		bool close()
		{
			return m_socket.close();
		}

		uint32_t getState() const
		{
			return m_state.load();
		}

        void addRef()
        {
            ++m_state;
        }

        void decRef()
        {
            if ((--m_state & 0xFF) == 0u)
            {
                delete this;
            }
        }

	private:
		virtual bool handleReadComplete() = 0;
		virtual bool handleWriteComplete() = 0;
		virtual bool handleAcceptComplete() = 0;
		virtual bool handleConnectComplete() = 0;
		virtual bool handleIOError() = 0;

	private:
		void* m_userData;

	protected:
		Socket m_socket;
		IOService* m_ioService;
		std::atomic<uint32_t> m_state;
	};
}

#endif // _IO_COMPONENT_HPP_
