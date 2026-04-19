#include "Platform.hpp"

#ifdef _WIN32

	#define WIN32_LEAN_AND_MEAN
	#include <io.h>
	#include <windows.h>

#else

	#include <sys/ioctl.h>
	#include <sys/mman.h>
	#include <unistd.h>

#endif

namespace Wandelt::Platform
{

#ifdef _WIN32

	int GetTerminalWidth(void)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
			return csbi.srWindow.Right - csbi.srWindow.Left + 1;
		return 80;
	}

	bool IsDebuggerPresent()
	{
		return ::IsDebuggerPresent() != FALSE;
	}

	void* VMemReserve(u64 size)
	{
		return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
	}

	void* VMemCommit(void* base, u64 size)
	{
		return VirtualAlloc(base, size, MEM_COMMIT, PAGE_READWRITE);
	}

	void VMemRelease(void* base, u64 /*size*/)
	{
		VirtualFree(base, 0, MEM_RELEASE);
	}

#else

	int GetTerminalWidth(void)
	{
		struct winsize w;
		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
			return (int)w.ws_col;
		return 80;
	}

	bool IsDebuggerPresent()
	{
		return false;
	}

	void* VMemReserve(u64 size)
	{
		// Reserves virtual address space without backing it with physical memory.
		void* ptr = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
		if (ptr == MAP_FAILED)
			return nullptr;

		return ptr;
	}

	void* VMemCommit(void* base, u64 size)
	{
		// mprotect mirrors Windows' commit by enabling read/write access.
		if (mprotect(base, size, PROT_READ | PROT_WRITE) != 0)
			return nullptr;

		return base;
	}

	void VMemRelease(void* base, u64 size)
	{
		munmap(base, size);
	}

#endif

} // namespace Wandelt::Platform
