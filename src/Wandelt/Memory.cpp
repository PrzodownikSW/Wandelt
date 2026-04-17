#include "Memory.hpp"

#include <cstring>

namespace Wandelt
{

	void* HeapAllocator::Alloc(u64 size)
	{
		void* memory = std::calloc(1, size);
		ASSERT(memory != nullptr, "Failed to allocate memory!");

		return memory;
	}

	void* HeapAllocator::Realloc(void* ptr, u64 /*oldSize*/, u64 newSize)
	{
		void* memory = std::realloc(ptr, newSize);
		ASSERT(memory != nullptr, "Failed to reallocate memory!");

		return memory;
	}

	void HeapAllocator::Free(void* ptr, u64 /*size*/)
	{
		std::free(ptr);
	}

	HeapAllocator GetHeapAllocator()
	{
		return HeapAllocator();
	}

	ArenaAllocator GetArenaAllocator(Allocator* baseAllocator, u64 arenaSize)
	{
		return ArenaAllocator(baseAllocator, arenaSize);
	}

	ArenaAllocator::ArenaAllocator(Allocator* baseAllocator, u64 arenaSize) : m_BaseAllocator(baseAllocator), m_Size(arenaSize)
	{
		ASSERT(baseAllocator != nullptr, "Allocator is nullptr!");
		ASSERT(arenaSize > 0, "Cannot create allocator with %llu size!", arenaSize);
		ASSERT(arenaSize % 16 == 0, "Allocator size must be a multiple of 16 bytes!");

		m_Memory = m_BaseAllocator->Alloc(m_Size);
		ASSERT(m_Memory != nullptr, "Failed to allocate memory for arena!");
	}

	ArenaAllocator::~ArenaAllocator()
	{
		m_BaseAllocator->Free(m_Memory, m_Size);
	}

	void* ArenaAllocator::Alloc(u64 size)
	{
		ASSERT(size > 0, "Cannot allocate 0 bytes!");

		size = (size + 15u) & ~(u64)15;

		ASSERT(m_Used + size <= m_Size, "Arena out of memory! Used: %llu bytes, Requested: %llu bytes, Total: %llu bytes", m_Used, size, m_Size);

		void* memory = static_cast<u8*>(m_Memory) + m_Used;
		m_Used += size;
		m_Allocations++;

		return memory;
	}

	void* ArenaAllocator::Realloc(void* ptr, u64 oldSize, u64 newSize)
	{
		ASSERT(ptr != nullptr, "Cannot reallocate nullptr!");
		ASSERT(oldSize > 0, "Old size must be greater than 0!");
		ASSERT(newSize > 0, "New size must be greater than 0!");
		ASSERT(newSize >= oldSize, "Arena allocator does not support shrinking reallocations!");

		void* newMemory = Alloc(newSize);
		std::memcpy(newMemory, ptr, oldSize);
		return newMemory;
	}

	void ArenaAllocator::Free(void* /*ptr*/, u64 /*size*/)
	{
		// Arena allocator does not support freeing individual allocations
	}

	void ArenaAllocator::Reset()
	{
		m_Used        = 0;
		m_Allocations = 0;
	}

} // namespace Wandelt
