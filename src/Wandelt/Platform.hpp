#pragma once

#include "Wandelt/Defines.hpp"

namespace Wandelt::Platform
{

	int GetTerminalWidth();
	bool IsDebuggerPresent();

	void* VMemReserve(u64 size);
	void* VMemCommit(void* base, u64 size);
	void VMemRelease(void* base, u64 size);

} // namespace Wandelt::Platform
