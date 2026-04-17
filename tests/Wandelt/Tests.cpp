#include "Tests.hpp"

#include "Wandelt/ScopedTimer.hpp"

#include "LexerTests.hpp"

namespace Wandelt
{

	int RunTests(const char* filter)
	{
		SetTestFilter(filter);

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

		f64 grand_ms = timer.GetElapsedMilliseconds();

		printf("\n");
		if (totalRun == 0 && filter && filter[0] != '\0')
		{
			printf(ANSI_COLOR_YELLOW ANSI_COLOR_BOLD "No tests matched filter '%s'" ANSI_COLOR_RESET "\n", filter);
			return 1;
		}

		if (totalFailed == 0)
		{
			printf(ANSI_COLOR_GREEN ANSI_COLOR_BOLD "All %d tests passed" ANSI_COLOR_RESET ANSI_COLOR_DIM
			                                        "  (%d suite%s, %.2fms total)" ANSI_COLOR_RESET "\n",
			       totalRun, suitesRun, suitesRun == 1 ? "" : "s", grand_ms);
		}
		else
		{
			printf(ANSI_COLOR_RED ANSI_COLOR_BOLD "%d of %d tests failed" ANSI_COLOR_RESET ANSI_COLOR_DIM
			                                      "  (%d/%d suites failed, %.2fms total)" ANSI_COLOR_RESET "\n",
			       totalFailed, totalRun, suitesFailed, suitesRun, grand_ms);
			printf(ANSI_COLOR_GREEN "%d passed" ANSI_COLOR_RESET ", " ANSI_COLOR_RED "%d failed" ANSI_COLOR_RESET "\n", totalPassed, totalFailed);
		}

		return totalFailed > 0 ? 1 : 0;
	}

} // namespace Wandelt
