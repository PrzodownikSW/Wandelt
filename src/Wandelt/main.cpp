#include <cstring>
#include <iostream>

#include "Wandelt/Defines.hpp"
#include "Wandelt/Memory.hpp"
#include "Wandelt/Parser.hpp"
#include "Wandelt/ScopedTimer.hpp"
#include "Wandelt/Sema.hpp"
#include "Wandelt/String.hpp"

using namespace Wandelt;

#if 1
	#include "Wandelt/Tests.hpp"
#endif

static void PrintHelp(std::ostream& stream, const char* programName)
{
	stream << "Usage: " << programName << " [options]\n\n";
	stream << "General options:\n";
	stream << "  -help         Show this help message and exit.\n";
	stream << "  -no-colors    Disable ANSI colors in diagnostics and test output.\n";
	stream << "  -test         Run the test suite.\n\n";
	stream << "Test options:\n";
	stream << "  -filter=NAME  Run only the named test.\n\n";
	stream << "With no options, Wandelt lexes the demo source file.\n";
}

int main(int argc, char* argv[])
{
	bool runTests      = false;
	bool useColors     = true;
	const char* filter = nullptr;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-test") == 0)
			runTests = true;
		else if (strcmp(argv[i], "-help") == 0)
		{
			PrintHelp(std::cout, argv[0]);
			return 0;
		}
		else if (strcmp(argv[i], "-no-colors") == 0)
			useColors = false;
		else if (strncmp(argv[i], "-filter=", 8) == 0)
			filter = argv[i] + 8;
		else
		{
			std::cerr << "Unknown argument: " << argv[i] << "\n\n";
			PrintHelp(std::cerr, argv[0]);
			return 1;
		}
	}

#if 1
	if (runTests)
	{
		return RunTests(filter, useColors);
	}
#endif

	VirtualMemoryAllocator heapAllocator(Megabytes(10));
	ArenaAllocator typeArenaAllocator(&heapAllocator, Megabytes(1));
	ArenaAllocator stringArenaAllocator(&heapAllocator, Megabytes(1));
	ArenaAllocator stmtArenaAllocator(&heapAllocator, Megabytes(1));
	ArenaAllocator declArenaAllocator(&heapAllocator, Megabytes(1));
	ArenaAllocator exprArenaAllocator(&heapAllocator, Megabytes(1));

	Type::Initialize(&typeArenaAllocator);

	String demo_filepath = String::FromCStr(&stringArenaAllocator, DEMO_PATH "main.wdt");
	File demo_file{&stringArenaAllocator, std::move(demo_filepath)};

	f64 dtLexingParsing = 0.0;
	f64 dtSema          = 0.0;

	Diagnostics* diagnostics = new Diagnostics();
	defer(delete diagnostics);
	diagnostics->SetUseColor(useColors);

	TranslationUnit tu;

	{
		ScopedTimer timer;
		Lexer lexer{&demo_file, diagnostics};
		Parser parser{&stmtArenaAllocator, &declArenaAllocator, &exprArenaAllocator, &lexer, diagnostics};
		tu = parser.Parse();

		dtLexingParsing = timer.GetElapsedMilliseconds();

		std::cout << "Lexing and parsing took " << dtLexingParsing << " ms" << std::endl;
	}

	if (diagnostics->HasErrors())
	{
		std::cerr << "Compilation failed with " << diagnostics->ErrorCount() << " error(s) and " << diagnostics->WarningCount() << " warning(s).\n";
		return 1;
	}

	{
		ScopedTimer timer;
		Sema sema(&declArenaAllocator, &exprArenaAllocator, &tu, diagnostics);
		if (!sema.Analyze())
		{
			std::cerr << "Semantic analysis failed with " << diagnostics->ErrorCount() << " error(s) and " << diagnostics->WarningCount()
			          << " warning(s).\n";
			return 1;
		}

		dtSema = timer.GetElapsedMilliseconds();

		std::cout << "Semantic analysis took " << dtSema << " ms" << std::endl;
	}

	if (diagnostics->HasErrors())
	{
		std::cerr << "Compilation failed with " << diagnostics->ErrorCount() << " error(s) and " << diagnostics->WarningCount() << " warning(s).\n";
		return 1;
	}

	std::cout << "Compilation succeeded with " << diagnostics->WarningCount() << " warning(s).\n";

	return 0;
}
