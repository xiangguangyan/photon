#ifndef _QUEUE_HPP_
#define _QUEUE_HPP_

namespace photon
{
	template<typename T, uint32_t Size>
	class Queue
	{
	public:
		Queue() :
			m_in(0),
			m_out(0)
		{

		}

		bool push(const T& object)
		{
			if (full())
			{
				return false;
			}

			m_objects[m_in & (Size - 1)] = object;
			++m_in;
			return true;
		}

		bool pop(T& object)
		{
			if (empty())
			{
				return false;
			}

			object = m_objects[m_out & (Size - 1)];
			++m_out;
			return true;
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
		uint32_t m_in;
		uint32_t m_out;
		typename std::enable_if<Size != 0 && (Size & (Size - 1)) == 0, T>::type m_objects[Size];
	};
}

#endif