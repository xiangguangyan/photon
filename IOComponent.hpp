#ifndef _IO_COMPONENT_HPP_
#define _IO_COMPONENT_HPP_

#include <stdint.h>
#include <atomic>

#include "Socket.hpp"

namespace photon
{
	class IOService;
	class IOCPService;
}

namespace photon
{
	class IOComponent
	{
		friend class IOService;
		friend class IOCPService;

	public:

		//iocp��WSARecvʱ��READING״̬λ����ɺ���READ_COMPLETE״̬λ�����READING״̬λ�������ն���δ�������WSARecv��������READ_QUEUE_FULL״̬λ��
		//epoll��readʱ��READING״̬λ��EWOULDBLOCKʱ��READ_COMPLETE״̬λ�����READING״̬λ���������ն�����������READ_COMPLETE��READ_QUEUE_FULL״̬λ��
		//handleReadComplete���������ύ���û������ж�READ_QUEUE_FULL״̬λ���类��λ����Ҫ����IOService��startRead

		//iocp��WSASendʱ��WRITING״̬λ����ɺ���WRITE_COMPLETE״̬λ�����WRITING״̬λ�������Ͷ���δ�������WSASend��������WRITE_BUFFER_EMPTY״̬λ��
		//epoll��writeʱ��WRITING״̬λ��EWOULDBLOCKʱ��WRITE_COMPLETE״̬λ�����WRITING״̬λ������䷢�Ͷ����ȿգ���WRITE_COMPLETE��WRITE_QUEUE_EMPTY״̬λ;
		//postPacket���ڽ����û��ύ�����ݺ����ж�WRITE_QUEUE_EMPTY״̬λ���类��λ����Ҫ����IOService��startWrite

		//IO����Է��رջ��߳���ʱ��IOERROR״̬λ
		enum IOComponentState
		{
			READING = 1,
			WRITING = 2,
			ACCEPTING = 4,
			CONNECTING = 8,

			CONNECTED = 1024,

			IOERROR = 2048
		};

	public:
		IOComponent(IOService* ioService, int addressFamily, int type, int protocol = 0) :
			m_ioService(ioService),
			m_state(0u),
			m_userData(nullptr),
			m_socket(addressFamily, type, protocol)
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
