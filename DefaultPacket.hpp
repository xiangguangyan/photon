#ifndef _DEFAULT_PACKET_HPP_
#define _DEFAULT_PACKET_HPP_

#include "Packet.hpp"

#include <string>

namespace photon
{
	class DefaultPacket : public Packet
	{
	public:
		virtual uint32_t getSize() const
		{
			return ReadBufferQueue::getSize(m_message);
		}

		virtual void serialize(WriteBufferQueue& writeQueue) const override
		{
			writeQueue << m_message;
		}

		virtual void deserialize(ReadBufferQueue& readQueue) override
		{
			readQueue >> m_message;
		}

	public:
		std::string m_message;
	};
}

#endif //_DEFAULT_PACKET_HPP_