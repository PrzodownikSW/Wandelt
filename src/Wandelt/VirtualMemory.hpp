#pragma once

#include "Wandelt/Defines.hpp"

namespace Wandelt
{

	class VirtualMemory
	{
	public:
		explicit VirtualMemory(u64 sizeInBytes);
		~VirtualMemory();

		NONCOPYABLE(VirtualMemory);
		NONMOVABLE(VirtualMemory);

	public:
		u64 GetSize() const { return m_Size; }
		u64 GetAllocated() const { return m_Allocated; }
		u64 GetCommitted() const { return m_Committed; }

	public:
		void* Allocate(u64 sizeInBytes);
		void Reset();

	private:
		void* m_Handle  = nullptr;
		u64 m_Size      = 0;
		u64 m_Allocated = 0;
		u64 m_Committed = 0;
	};

} // namespace Wandelt
