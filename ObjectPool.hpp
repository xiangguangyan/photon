#ifndef _OBJECT_POOL_HPP_
#define _OBJECT_POOL_HPP_

namespace photon
{
	template<typename T, uint32_t Size>
	class ObjectPool
	{
		T* allocate();
		bool free(T* object);


	private:
		T m_objects[Size];
	};
}

#endif //_OBJECT_POOL_HPP_