#ifndef _BUFFER_QUEUE_HPP_
#define _BUFFER_QUEUE_HPP_

#include "Buffer.hpp"

#include <stdint.h>
#include <string>
#include <string.h>
#include <mutex>
#include <condition_variable>
#include <type_traits>

namespace photon
{
	template<uint32_t Length>
	class BufferQueue
	{
	public:
		BufferQueue() :
			m_in(0u),
			m_out(0u)
		{

		}

		BufferQueue& operator<<(const int8_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator<<(const uint8_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator<<(const int16_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator<<(const uint16_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator<<(const int32_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator<<(const uint32_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator<<(const int64_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator<<(const uint64_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator<<(const float& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator<<(const double& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator<<(const Buffer& value)
		{
			*this << value.m_length;
			push(value.m_buffer, value.m_length);
			return *this;
		}

		BufferQueue& operator<<(const std::string& value)
		{
			*this << (uint32_t)value.length();
			push(value.c_str(), (uint32_t)value.length());
			return *this;
		}


		BufferQueue& operator>>(int8_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator>>(uint8_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator>>(int16_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator>>(uint16_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator>>(int32_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator>>(uint32_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator>>(int64_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator>>(uint64_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator>>(float& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		BufferQueue& operator>>(double& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return this;
		}

		BufferQueue& operator>>(Buffer& value)
		{
			*this >> value.m_length;
			pop(value.m_buffer, value.m_length);
			return *this;
		}

		BufferQueue& operator>>(std::string& value)
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
			return (uint32_t)sizeof(uint32_t) + (uint32_t)value.size();
		}

		bool push(const char* buffer, uint32_t length)
		{
			while (freeSize() < length)
			{
				//���п��пռ䲻�㣬��Ҫ�ȴ�ȡ���ݣ������㹻�Ŀռ�
				return false;
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

			return true;
		}

		bool pop(char* buffer, uint32_t length)
		{
			while (dataSize() < length)
			{
				//�������ݲ��㣬��Ҫ�ȴ���������
				return false;
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

			return true;
		}

		const Buffer front() const
		{
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

		bool tryLock()
		{
			return m_mutex.try_lock();
		}

		void unlock()
		{
			m_mutex.unlock();
		}

	private:
		uint32_t m_in;
		uint32_t m_out;
		typename std::enable_if<Length != 0 && (Length & (Length - 1)) == 0, char>::type m_buffer[Length];
		std::mutex m_mutex;
	};

	typedef BufferQueue<1024 * 256> ReadBufferQueue;
	typedef BufferQueue<1024 * 256> WriteBufferQueue;
}

#endif //_BUFFER_QUEUE_HPP_