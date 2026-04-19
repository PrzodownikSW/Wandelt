#include "VirtualMemory.hpp"

#include "Wandelt/Platform.hpp"

namespace Wandelt
{

	static constexpr u64 COMMIT_PAGE_SIZE = 0x10000;

	VirtualMemory::VirtualMemory(u64 sizeInBytes)
	{
		void* ptr = Platform::VMemReserve(sizeInBytes);
		ASSERT(ptr != nullptr, "Failed to map virtual memory block!");

		m_Handle = ptr;
		m_Size   = sizeInBytes;
	}

	VirtualMemory::~VirtualMemory()
	{
		Platform::VMemRelease(m_Handle, m_Size);
	}

	void* VirtualMemory::Allocate(u64 sizeInBytes)
	{
		const u64 allocatedAfter   = m_Allocated + sizeInBytes;
		const u64 blocksCommitted  = m_Committed / COMMIT_PAGE_SIZE;
		const u64 endBlock         = (allocatedAfter + COMMIT_PAGE_SIZE - 1) / COMMIT_PAGE_SIZE;
		const u64 blocksToAllocate = endBlock - blocksCommitted;

		if (blocksToAllocate > 0)
		{
			const u64 toCommit = blocksToAllocate * COMMIT_PAGE_SIZE;

			void* result = Platform::VMemCommit(static_cast<u8*>(m_Handle) + m_Committed, toCommit);
			ASSERT(result != nullptr, "Failed to commit more virtual memory!");

			m_Committed += toCommit;
		}

		void* ptr   = static_cast<u8*>(m_Handle) + m_Allocated;
		m_Allocated = allocatedAfter;
		ASSERT(m_Allocated <= m_Size, "Out of virtual memory! Used: %llu bytes, Total: %llu bytes", m_Allocated, m_Size);

		return ptr;
	}

	void VirtualMemory::Reset()
	{
		// Keep committed pages around for reuse; only the bump offset resets.
		m_Allocated = 0;
	}

} // namespace Wandelt
