#include "Platform.hpp"

namespace Wandelt::Platform
{

#ifdef _WIN32

	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>

	int GetTerminalWidth(void)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
			return csbi.srWindow.Right - csbi.srWindow.Left + 1;
		return 80;
	}

#else

	int GetTerminalWidth(void)
	{
		struct winsize w;
		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
			return (int)w.ws_col;
		return 80;
	}

#endif

} // namespace Wandelt::Platform
