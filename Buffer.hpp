#ifndef _BUFFER_HPP_
#define _BUFFER_HPP_

#include <stdint.h>

namespace photon
{
	class Buffer
	{
		template<uint32_t Length>
		friend class SyncBufferQueue;

	public:
		Buffer() :
			m_buffer(nullptr),
			m_length(0)
		{

		}

		Buffer(char* buffer, uint32_t length) :
			m_buffer(buffer),
			m_length(length)
		{

		}

		template<uint32_t Length>
		Buffer(char(&buffer)[Length]) :
			m_buffer(buffer),
			m_length(Length)
		{

		}

		char* getData()
		{
			return m_buffer;
		}

		const char* getData() const
		{
			return m_buffer;
		}

		uint32_t getSize() const
		{
			return m_length;
		}

		operator bool() const
		{
			return nullptr != m_buffer && 0 != m_length;
		}

	private:
		char* m_buffer;
		uint32_t m_length;
	};
}

#endif //_BUFFER_HPP_