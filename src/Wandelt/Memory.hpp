#pragma once

#include "Wandelt/Defines.hpp"
#include "Wandelt/VirtualMemory.hpp"

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

		template <typename T>
		T* Alloc()
		{
			return static_cast<T*>(Alloc(sizeof(T)));
		}
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

	class VirtualMemoryAllocator final : public Allocator
	{
	public:
		NONCOPYABLE(VirtualMemoryAllocator);

		explicit VirtualMemoryAllocator(u64 reservedSize);
		~VirtualMemoryAllocator() override = default;

	public:
		u64 GetUsed() const { return m_Used; }
		u64 GetAllocations() const { return m_Allocations; }
		u64 GetReservedSize() const { return m_Memory.GetSize(); }
		u64 GetCommittedSize() const { return m_Memory.GetCommitted(); }

	public:
		void* Alloc(u64 size) override;
		void* Realloc(void* ptr, u64 oldSize, u64 newSize) override;
		void Free(void* ptr, u64 size) override;
		void Reset() override;

	private:
		VirtualMemory m_Memory;
		u64 m_Used        = 0;
		u64 m_Allocations = 0;
	};

} // namespace Wandelt
