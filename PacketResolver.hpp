#ifndef _PACKET_RESOLVER_HPP_
#define _PACKET_RESOLVER_HPP_

#include "Packet.hpp"
#include "BufferQueue.hpp"
#include "DefaultPacket.hpp"

namespace photon
{
	class PacketResolver
	{
    private:
        enum :uint32_t { MAGIC_NUM = 0x12345678 };

	public:
		virtual bool getPacketHeader(PacketHeader& header, ReadBufferQueue& readQueue) const
		{
			if (readQueue.dataSize() < sizeof(MAGIC_NUM)+sizeof(header.m_type) + sizeof(header.m_chanelId) + sizeof(header.m_size))
			{
				return false;
			}

			uint32_t magicNum = 0;
			readQueue >> magicNum;
			if (magicNum != MAGIC_NUM)
			{
				std::cout << "Invalid magic number:" << magicNum << std::endl;
				return false;
			}

			readQueue >> header.m_type >> header.m_chanelId >> header.m_size;

			return true;
		}

		virtual bool serialize(const Packet* packet, WriteBufferQueue& writeQueue) const
		{
			if (writeQueue.freeSize() < sizeof(MAGIC_NUM)+sizeof(packet->m_header.m_type)
				+ sizeof(packet->m_header.m_chanelId) + sizeof(packet->m_header.m_size) + packet->getSize())
			{
				return false;
			}

			writeQueue << MAGIC_NUM << packet->m_header.m_type << packet->m_header.m_chanelId;
			uint32_t size = packet->getSize();
			writeQueue << size;

			uint32_t oldSize = writeQueue.dataSize();
			packet->serialize(writeQueue);
			if (writeQueue.dataSize() - oldSize != size)
			{
				std::cout << "Packet getSize:" << size << " is not equal to serialize size:" << writeQueue.dataSize() - oldSize << std::endl;
				return false;
			}

			return true;
		}

		virtual Packet* deserialize(const PacketHeader& header, ReadBufferQueue& readQueue)
		{
			if (readQueue.dataSize() < header.m_size)
			{
				return nullptr;
			}

			Packet* packet = createPacket(header);

			uint32_t oldSize = readQueue.dataSize();
			packet->deserialize(readQueue);

			if (oldSize - readQueue.dataSize() != header.m_size)
			{
				std::cout << "Packet deserialize size:" << oldSize - readQueue.dataSize() << " is not equal to PacketHeader.size:" << header.m_size << std::endl;
				freePacket(packet);
				return nullptr;
			}

			packet->m_header = header;
			return packet;
		}

		virtual Packet* createPacket(const PacketHeader& header)
		{
			return new DefaultPacket();
		}

		virtual void freePacket(const Packet* packet)
		{
			delete packet;
		}
	};
}

#endif // _PACKET_RESOLVER_HPP_
