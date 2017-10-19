#ifndef _PACKET_HPP_
#define _PACKET_HPP_

#include "BufferQueue.hpp"

namespace photon
{
	class PacketHeader
	{
	public:
		PacketHeader() :
			m_type(0),
			m_chanelId(0),
			m_size(0)
		{

		}

	public:
		uint32_t m_type;
		uint32_t m_chanelId;
		uint32_t m_size;
	};

	class Packet
	{
	public:
		virtual uint32_t getSize() const = 0;
		virtual void serialize(WriteBufferQueue& writeQueue) const = 0;
		virtual void deserialize(ReadBufferQueue& readQueue) = 0;

	public:
		PacketHeader m_header;			//用户只需要给header中m_type赋值，不需要对header进行序列化和反序列化
	};
}

#endif //_PACKET_HPP_