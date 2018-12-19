#ifndef _WAIT_FREE_BUFFER_QUEUE_HPP_
#define _WAIT_FREE_BUFFER_QUEUE_HPP_

#include "Buffer.hpp"

#include <stdint.h>
#include <string>
#include <string.h>
#include <mutex>
#include <atomic>
#include <type_traits>

namespace photon
{
	template<uint32_t Length>
	class WaitFreeBufferQueue
	{
	public:
		WaitFreeBufferQueue() :
			m_preIn(0u),
            m_postIn(0u),
			m_preOut(0u),
            m_postOut(0u)
		{

		}

		WaitFreeBufferQueue& operator<<(const int8_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator<<(const uint8_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator<<(const int16_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator<<(const uint16_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator<<(const int32_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator<<(const uint32_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator<<(const int64_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator<<(const uint64_t& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator<<(const float& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator<<(const double& value)
		{
			push(reinterpret_cast<const char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator<<(const Buffer& value)
		{
			*this << value.m_length;
			push(value.m_buffer, value.m_length);
			return *this;
		}

		WaitFreeBufferQueue& operator<<(const std::string& value)
		{
			*this << (uint32_t)value.length();
			push(value.c_str(), (uint32_t)value.length());
			return *this;
		}


		WaitFreeBufferQueue& operator>>(int8_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator>>(uint8_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator>>(int16_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator>>(uint16_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator>>(int32_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator>>(uint32_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator>>(int64_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator>>(uint64_t& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator>>(float& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return *this;
		}

		WaitFreeBufferQueue& operator>>(double& value)
		{
			pop(reinterpret_cast<char*>(&value), sizeof(value));
			return this;
		}

		WaitFreeBufferQueue& operator>>(Buffer& value)
		{
			*this >> value.m_length;
			pop(value.m_buffer, value.m_length);
			return *this;
		}

		WaitFreeBufferQueue& operator>>(std::string& value)
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

		void push(const char* buffer, uint32_t length)
		{
            uint32_t preIn = m_preIn.fetch_add(length);         //take position

            while (preIn - m_postOut + length > Length);      //wait readers read

			uint32_t backFreeStart = preIn & (Length - 1);
			uint32_t backFreeToEndLen = Length - backFreeStart;

			if (backFreeToEndLen >= length)
			{
				//队列剩余空间连续,直接填充数据
				memcpy(m_buffer + backFreeStart, buffer, length);
			}
			else
			{
				//剩余空间不连续，先填满队列尾部空间
				memcpy(m_buffer + backFreeStart, buffer, backFreeToEndLen);

				//再将剩余部分填充至头部
				memcpy(m_buffer, buffer + backFreeToEndLen, length - backFreeToEndLen);
			}

            while (m_postIn < preIn);                           //wait previous writer finish

            m_postIn += length;
		}

		void pop(char* buffer, uint32_t length)
		{
            uint32_t preOut = m_preOut.fetch_add(length);   //take position

            while (m_postIn - preOut < length);             //wait writers write

			uint32_t backDataStart = preOut & (Length - 1);
			uint32_t backDataToEndLen = Length - backDataStart;

			if (backDataToEndLen >= length)
			{
				//队列数据空间连续,直接填充数据
				memcpy(buffer, m_buffer + backDataStart, length);
			}
			else
			{
				//数据空间不连续，先使用队列尾部数据
				memcpy(buffer, m_buffer + backDataStart, backDataToEndLen);

				//再使用队列头部数据
				memcpy(buffer + backDataToEndLen, m_buffer, length - backDataToEndLen);
			}

            while (m_postOut < preOut);

            m_postOut += length;
		}

		/*const Buffer front() const
		{
			if (empty())
			{
				return Buffer();
			}

			uint32_t dataStart = m_out & (Length - 1);
			uint32_t dataEnd = m_in & (Length - 1);

			if (dataStart < dataEnd)
			{
				return Buffer(const_cast<char*>(m_buffer) + dataStart, dataEnd - dataStart);
			}

			return Buffer(const_cast<char*>(m_buffer) + dataStart, Length - dataStart);
		}

		void pop(uint32_t length)
		{
			m_out += length;
		}

		Buffer reserve() noexcept
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

		void push(uint32_t length) noexcept
		{
			m_in += length;
		}*/

		bool empty() const noexcept
		{
			return m_preIn == m_postOut;
		}

		bool full() const noexcept
		{
			return m_preIn - m_postOut == Length;
		}

		uint32_t totalSize() const noexcept
		{
			return Length;
		}

		uint32_t dataSize() const noexcept
		{
			return m_postIn - m_preOut;
		}

		uint32_t freeSize() const noexcept
		{
			return Length - dataSize();
		}

	private:
        std::atomic<uint32_t> m_preIn;
        std::atomic<uint32_t> m_postIn;
		std::atomic<uint32_t> m_preOut;
        std::atomic<uint32_t> m_postOut;
		typename std::enable_if<Length != 0 && (Length & (Length - 1)) == 0, char>::type m_buffer[Length];
	};
}

#endif //_WAIT_FREE_BUFFER_QUEUE_HPP_