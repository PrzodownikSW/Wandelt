#include "Wandelt/Defines.hpp"
#include "Wandelt/Diagnostics.hpp"
#include "Wandelt/File.hpp"
#include "Wandelt/Lexer.hpp"
#include "Wandelt/Memory.hpp"
#include "Wandelt/ScopedTimer.hpp"
#include "Wandelt/String.hpp"

using namespace Wandelt;

#if 1
	#include "Wandelt/Tests.hpp"
int RunTestCases(const char* filter)
{
	return RunTests(filter);
}
#endif

int main(int argc, char* argv[])
{
	bool runTests      = false;
	const char* filter = nullptr;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-test") == 0)
			runTests = true;
		else if (strncmp(argv[i], "-filter=", 8) == 0)
			filter = argv[i] + 8;
	}

#if 1
	if (runTests)
	{
		return RunTestCases(filter);
	}
#endif

	HeapAllocator heapAllocator         = GetHeapAllocator();
	ArenaAllocator stringArenaAllocator = GetArenaAllocator(&heapAllocator, Megabytes(1));
	ArenaAllocator stmtArenaAllocator   = GetArenaAllocator(&heapAllocator, Megabytes(1));
	ArenaAllocator declArenaAllocator   = GetArenaAllocator(&heapAllocator, Megabytes(1));
	ArenaAllocator exprArenaAllocator   = GetArenaAllocator(&heapAllocator, Megabytes(1));

	String demo_filepath = String::FromCStr(&stringArenaAllocator, DEMO_PATH "main.wdt");
	File demo_file{&stringArenaAllocator, std::move(demo_filepath)};

	double dtLexingParsing = 0.0;

	Diagnostics* diagnostics = new Diagnostics();
	defer(delete diagnostics);

	{
		ScopedTimer timer;
		Lexer lexer{&demo_file, diagnostics};

		while (lexer.PeekToken().type != TOKEN_TYPE_EOF)
		{
			lexer.DebugPrintToken(lexer.PeekToken());
			lexer.EatToken();
		}

		dtLexingParsing = timer.GetElapsedMilliseconds();

		std::cout << "Lexing and parsing took " << dtLexingParsing << " ms" << std::endl;
	}

	return 0;
}
