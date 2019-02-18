#ifndef _WAIT_FREE_QUEUE_HPP_
#define _WAIT_FREE_QUEUE_HPP_

#include <atomic>

namespace photon
{
	template<typename T, uint32_t Size>
	class WaitFreeQueue
	{
	public:
		WaitFreeQueue() :
			m_in(0),
			m_out(0)
		{

		}

		uint32_t getWriteIdx()
		{
			return m_in.fetch_add(1, std::memory_order_relaxed);
		}

		T* getWritable(uint32_t idx)
		{
			Object& object = m_objects[idx % Size];
			if (object.filled.load(std::memory_order_acquire))
			{
				return nullptr;
			}
			return reinterpret_cast<T*>(&object.data);
		}

		void commitWrite(uint32_t idx) 
		{
			Object& object = m_objects[idx % Size];
			object.filled.store(true, std::memory_order_release);
		}

		uint32_t  getReadIdx()
		{
			return m_out.fetch_add(1, std::memory_order_relaxed);
		}

		const T* getReadable(uint32_t idx)
		{
			Object& object = m_objects[idx % Size];
			if (object.filled.load(std::memory_order_acquire))
			{
				return reinterpret_cast<T*>(&object.data);
			}
			return nullptr;
		}

		void commitRead(uint32_t idx)
		{
			Object& object = m_objects[idx % Size];
			reinterpret_cast<T&>(object.data).~T();
			object.filled.store(false, std::memory_order_release);
		}

		bool empty() const
		{
			return m_in == m_out;
		}

		bool full() const
		{
			return m_in - m_out == Size;
		}

		uint32_t totalSize() const
		{
			return Size;
		}

		uint32_t dataSize() const
		{
			return m_in - m_out;
		}

		uint32_t freeSize() const
		{
			return Size - dataSize();
		}

	private:
		std::atomic<uint32_t> m_in;
		std::atomic<uint32_t> m_out;
		
		struct Object
		{
			Object() :
				filled(false)
			{

			}

			std::atomic<bool> filled;
			T data;
		};

		typename std::enable_if<Size != 0 && (Size & (Size - 1)) == 0, Object>::type m_objects[Size];
	};
}

#endif	//_WAIT_FREE_QUEUE_HPP_
