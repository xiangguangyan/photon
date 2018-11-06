#ifndef _SYNC_BUFFER_QUEUE_HPP_
#define _SYNC_BUFFER_QUEUE_HPP_

#include "Buffer.hpp"

#include <stdint.h>
#include <string>
#include <mutex>
#include <condition_variable>
#include <type_traits>

namespace photon
{
	template<uint32_t Length>
	class SyncBufferQueue
	{
	public:
		SyncBufferQueue() :
			m_in(0u),
			m_out(0u),
			m_fullWait(false),
			m_emptyWait(false),
			m_fullSynCount(0u),
			m_emptySynCount(0u)
		{

		}

		SyncBufferQueue& operator<<(const int8_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator<<(const uint8_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator<<(const int16_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator<<(const uint16_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator<<(const int32_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator<<(const uint32_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator<<(const int64_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator<<(const uint64_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator<<(const float& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator<<(const double& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator<<(const Buffer& value)
		{
			*this << value.m_length;
			push(value.m_buffer, value.m_length);
			return *this;
		}

		SyncBufferQueue& operator<<(const std::string& value)
		{
			*this << (uint32_t)value.length();
			push(value.c_str(), (uint32_t)value.length());
			return *this;
		}


		SyncBufferQueue& operator>>(int8_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator>>(uint8_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator>>(int16_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator>>(uint16_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator>>(int32_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator>>(uint32_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator>>(int64_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator>>(uint64_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator>>(float& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		SyncBufferQueue& operator>>(double& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return this;
		}

		SyncBufferQueue& operator>>(Buffer& value)
		{
			*this >> value.m_length;
			pop(value.m_buffer, value.m_length);
			return *this;
		}

		SyncBufferQueue& operator>>(std::string& value)
		{
			uint32_t length = 0u;
			*this >> length;
			value.resize(length);
			pop(&value[0], length);
			return *this;
		}

		inline static uint32_t getSize(const int8_t& value)
		{
			return sizeof(value);
		}

		inline static uint32_t getSize(const uint8_t& value)
		{
			return sizeof(value);
		}

		inline static uint32_t getSize(const int16_t& value)
		{
			return sizeof(value);
		}

		inline static uint32_t getSize(const uint16_t& value)
		{
			return sizeof(value);
		}

		inline static uint32_t getSize(const int32_t& value)
		{
			return sizeof(value);
		}

		inline static uint32_t getSize(const uint32_t& value)
		{
			return sizeof(value);
		}

		inline static uint32_t getSize(const int64_t& value)
		{
			return sizeof(value);
		}

		inline static uint32_t getSize(const uint64_t& value)
		{
			return sizeof(value);
		}

		inline static uint32_t getSize(const float& value)
		{
			return sizeof(value);
		}

		inline static uint32_t getSize(const double& value)
		{
			return sizeof(value);
		}

		inline static uint32_t getSize(const Buffer& value)
		{
			return sizeof(value.m_length) + value.m_length;
		}

		inline static uint32_t getSize(const std::string& value)
		{
			return sizeof(uint32_t)+value.size();
		}

		void push(const char* buffer, uint32_t length)
		{
			while (m_fullWait)
			{
				++m_fullSynCount;
				m_fullSynVariable.wait(m_mutex);
				--m_fullSynCount;
			}

			while (freeSize() < length)
			{
				//���п��пռ䲻�㣬��Ҫ�ȴ������߳�ȡ���ݣ������㹻�Ŀռ�
				m_fullWait = true;
				m_fullWaitVariable.wait(m_mutex);
				m_fullWait = false;
			}

			uint32_t backFreeStart = m_in & (Length - 1);
			uint32_t backFreeToEndLen = Length - backFreeStart;

			if (backFreeToEndLen >= length)
			{
				//����ʣ��ռ�����,ֱ���������
				memcpy(m_buffer + backFreeStart, buffer, length);
			}
			else
			{
				//ʣ��ռ䲻����������������β���ռ�
				memcpy(m_buffer + backFreeStart, buffer, backFreeToEndLen);

				//�ٽ�ʣ�ಿ�������ͷ��
				memcpy(m_buffer, buffer + backFreeToEndLen, length - backFreeToEndLen);
			}

			//���¶���βָ��
			m_in += length;
		}

		void pop(char* buffer, uint32_t length)
		{
			while (m_emptyWait)
			{
				//����������߳�����Ϊ���пն��ȴ�����Ҳ��Ҫ�ȴ�
				++m_emptySynCount;
				m_emptySynVariable.wait(m_mutex);
				--m_emptySynCount;
			}

			while (dataSize() < length)
			{
				//�������ݲ��㣬��Ҫ�ȴ������̷߳�������
				m_emptyWait = true;
				m_emptyWaitVariable.wait(m_mutex);
				m_emptyWait = false;
			}

			uint32_t backDataStart = m_out & (Length - 1);
			uint32_t backDataToEndLen = Length - backDataStart;

			if (backDataToEndLen >= length)
			{
				//�������ݿռ�����,ֱ���������
				memcpy(buffer, m_buffer + backDataStart, length);
			}
			else
			{
				//���ݿռ䲻��������ʹ�ö���β������
				memcpy(buffer, m_buffer + backDataStart, backDataToEndLen);

				//��ʹ�ö���ͷ������
				memcpy(buffer + backDataToEndLen, m_buffer, length - backDataToEndLen);
			}

			//���¶���ͷָ��
			m_out += length;
		}

		const Buffer front() const
		{
			//while (m_emptyWait)
			//{
			//	//����������߳�����Ϊ���пն��ȴ�����Ҳ��Ҫ�ȴ�
			//	++m_emptySynCount;
			//	m_emptySynVariable.wait(m_mutex);
			//	--m_emptySynCount;
			//}

			//while (empty())
			//{
			//	//�������ݲ��㣬��Ҫ�ȴ������̷߳�������
			//	m_emptyWait = true;
			//	m_emptyWaitVariable.wait(m_mutex);
			//	m_emptyWait = false;
			//}

			if (empty())
			{
				return Buffer();
			}

			uint32_t dataStart = m_out & (Length - 1);
			uint32_t dataEnd = m_in & (Length - 1);

			if (dataStart < dataEnd)
			{
				return  Buffer(const_cast<char*>(m_buffer)+dataStart, dataEnd - dataStart);
			}

			return  Buffer(const_cast<char*>(m_buffer)+dataStart, Length - dataStart);
		}

		void pop(uint32_t length)
		{
			m_out += length;
		}

		Buffer reserve()
		{
			//while (m_fullWait)
			//{
			//	//����������߳�����Ϊ���������ȴ�����Ҳ��Ҫ�ȴ�
			//	++m_fullSynCount;
			//	m_fullSynVariable.wait(m_mutex);
			//	--m_fullSynCount;
			//}

			//while (full())
			//{
			//	//���п��пռ䲻�㣬��Ҫ�ȴ������߳�ȡ���ݣ������㹻�Ŀռ�
			//	m_fullWait = true;
			//	m_fullWaitVariable.wait(m_mutex);
			//	m_fullWait = false;
			//}

			//������������ȴ������̴߳Ӷ�����ȡ����
			if (full())
			{
				return Buffer();
			}

			uint32_t freeStart = m_in & (Length - 1);
			uint32_t freeEnd = m_out & (Length - 1);

			if (freeStart < freeEnd)
			{
				return Buffer(m_buffer + freeStart, freeEnd - freeStart);
			}

			return Buffer(m_buffer + freeStart, Length - freeStart);
		}

		void push(uint32_t length)
		{
			m_in += length;
		}

		bool empty() const
		{
			return m_in == m_out;
		}

		bool full() const
		{
			return m_in - m_out == Length;
		}

		uint32_t totalSize() const
		{
			return Length;
		}

		uint32_t dataSize() const
		{
			return m_in - m_out;
		}

		uint32_t freeSize() const
		{
			return Length - dataSize();
		}

		void lock()
		{
			m_mutex.lock();
		}

		void unlock()
		{
			bool fullWait = m_fullWait;
			bool emptyWait = m_emptyWait;
			bool fullSyn = m_fullSynCount > 0;
			bool emptySyn = m_emptySynCount > 0;

			m_mutex.unlock();

			if (fullWait)
			{
				m_fullWaitVariable.notify_one();
			}
			else if (emptyWait)
			{
				m_emptyWaitVariable.notify_one();
			}
			else if (fullSyn)
			{
				m_fullSynVariable.notify_one();
			}
			else if (emptySyn)
			{
				m_emptySynVariable.notify_one();
			}
		}

		bool fullWait()
		{
			return m_fullWait;
		}

		bool emptyWait()
		{
			return m_emptyWait;
		}

		operator bool()
		{
			return m_in - m_out <= Length;
		}

	private:
		uint32_t m_in;
		uint32_t m_out;
		typename std::enable_if<Length != 0 && (Length & (Length - 1)) == 0, char>::type m_buffer[Length];
		std::mutex m_mutex;
		std::condition_variable_any m_fullWaitVariable;
		bool m_fullWait;
		std::condition_variable_any m_emptyWaitVariable;
		bool m_emptyWait;
		std::condition_variable_any m_fullSynVariable;
		uint32_t m_fullSynCount;
		std::condition_variable_any m_emptySynVariable;
		uint32_t m_emptySynCount;
	};
}

#endif //_SYNC_BUFFER_QUEUE_HPP_