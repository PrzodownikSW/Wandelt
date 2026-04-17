#pragma once

#include "Wandelt/Defines.hpp"

namespace Wandelt
{

	class Allocator
	{
	public:
		virtual ~Allocator() = default;

	public:
		virtual void* Alloc(u64 size)                              = 0;
		virtual void* Realloc(void* ptr, u64 oldSize, u64 newSize) = 0;
		virtual void Free(void* ptr, u64 size)                     = 0;
		virtual void Reset()                                       = 0; // Reset allocator state
	};

	class HeapAllocator final : public Allocator
	{
	public:
		void* Alloc(u64 size) override;
		void* Realloc(void* ptr, u64 oldSize, u64 newSize) override;
		void Free(void* ptr, u64 size) override;
		void Reset() override {}
	};

	class ArenaAllocator final : public Allocator
	{
	public:
		NONCOPYABLE(ArenaAllocator);

		explicit ArenaAllocator(Allocator* baseAllocator, u64 arenaSize);
		~ArenaAllocator() override;

	public:
		void* Alloc(u64 size) override;
		void* Realloc(void* ptr, u64 oldSize, u64 newSize) override;
		void Free(void* ptr, u64 size) override;
		void Reset() override;

	private:
		Allocator* m_BaseAllocator = nullptr;
		void* m_Memory             = nullptr;
		u64 m_Size                 = 0;
		u64 m_Used                 = 0;
		u64 m_Allocations          = 0;
	};

	HeapAllocator GetHeapAllocator();
	ArenaAllocator GetArenaAllocator(Allocator* baseAllocator, u64 arenaSize);

} // namespace Wandelt
