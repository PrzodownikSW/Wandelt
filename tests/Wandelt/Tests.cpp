#include "Tests.hpp"

#include "Wandelt/Memory.hpp"
#include "Wandelt/ScopedTimer.hpp"
#include "Wandelt/Type.hpp"

#include "LexerTests.hpp"
#include "ParserTests.hpp"
#include "SemaTests.hpp"

namespace Wandelt
{

	int RunTests(const char* filter, bool useColors)
	{
		SetTestFilter(filter);
		SetTestUseColor(useColors);

		VirtualMemoryAllocator heapAllocator(Megabytes(10));
		ArenaAllocator typeArenaAllocator(&heapAllocator, Megabytes(1));

		Type::Initialize(&typeArenaAllocator);

		ScopedTimer timer;

		int totalRun     = 0;
		int totalPassed  = 0;
		int totalFailed  = 0;
		int suitesRun    = 0;
		int suitesFailed = 0;

		TestResults r = RunLexerTests();
		totalRun += r.run;
		totalPassed += r.passed;
		totalFailed += r.failed;
		suitesRun++;
		if (r.failed > 0)
			suitesFailed++;

		r = RunParserTests();
		totalRun += r.run;
		totalPassed += r.passed;
		totalFailed += r.failed;
		suitesRun++;
		if (r.failed > 0)
			suitesFailed++;

		r = RunSemaTests();
		totalRun += r.run;
		totalPassed += r.passed;
		totalFailed += r.failed;
		suitesRun++;
		if (r.failed > 0)
			suitesFailed++;

		f64 grand_ms = timer.GetElapsedMilliseconds();

		printf("\n");
		if (totalRun == 0 && filter && filter[0] != '\0')
		{
			printf("%s%sNo tests matched filter '%s'%s\n", TestColor(ANSI_COLOR_YELLOW), TestColor(ANSI_COLOR_BOLD), filter,
			       TestColor(ANSI_COLOR_RESET));
			return 1;
		}

		if (totalFailed == 0)
		{
			printf("%s%sAll %d tests passed%s%s  (%d suite%s, %.2fms total)%s\n", TestColor(ANSI_COLOR_GREEN), TestColor(ANSI_COLOR_BOLD), totalRun,
			       TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM), suitesRun, suitesRun == 1 ? "" : "s", grand_ms,
			       TestColor(ANSI_COLOR_RESET));
		}
		else
		{
			printf("%s%s%d of %d tests failed%s%s  (%d/%d suites failed, %.2fms total)%s\n", TestColor(ANSI_COLOR_RED), TestColor(ANSI_COLOR_BOLD),
			       totalFailed, totalRun, TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM), suitesFailed, suitesRun, grand_ms,
			       TestColor(ANSI_COLOR_RESET));
			printf("%s%d passed%s, %s%d failed%s\n", TestColor(ANSI_COLOR_GREEN), totalPassed, TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_RED),
			       totalFailed, TestColor(ANSI_COLOR_RESET));
		}

		return totalFailed > 0 ? 1 : 0;
	}

} // namespace Wandelt
