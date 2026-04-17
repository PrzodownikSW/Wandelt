#pragma once

#include <cstdio>
#include <cstring>

#include "Wandelt/Diagnostics.hpp"
#include "Wandelt/Lexer.hpp"
#include "Wandelt/ScopedTimer.hpp"

namespace Wandelt
{

	struct TestResults
	{
		int run;
		int passed;
		int failed;
		double timeMs;
	};

	struct AssertionFailure
	{
		const char* file;
		int line;
		char message[2048];
	};

	inline int g_TestsRun                 = 0;
	inline int g_TestsPassed              = 0;
	inline int g_TestsFailed              = 0;
	inline AssertionFailure g_LastFailure = {};

	inline char g_TestFilter[128]   = "";
	inline char g_RerunCommand[256] = "Wandelt.exe -test";

	inline void ResetTestCounters()
	{
		g_TestsRun    = 0;
		g_TestsPassed = 0;
		g_TestsFailed = 0;
	}

	inline void SetTestFilter(const char* filter)
	{
		if (filter && filter[0] != '\0')
			snprintf(g_TestFilter, sizeof(g_TestFilter), "%s", filter);
		else
			g_TestFilter[0] = '\0';
	}

	inline void SetRerunCommand(const char* command)
	{
		if (command && command[0] != '\0')
			snprintf(g_RerunCommand, sizeof(g_RerunCommand), "%s", command);
	}

	inline bool TestMatchesFilter(const char* testName)
	{
		if (g_TestFilter[0] == '\0')
			return true;
		return strcmp(g_TestFilter, testName) == 0;
	}

#define TEST(name)                                                                                                                               \
	static void Test_##name(Allocator* alloc);                                                                                                   \
	static void RunTest_##name(Allocator* alloc)                                                                                                 \
	{                                                                                                                                            \
		if (!TestMatchesFilter(#name))                                                                                                           \
			return;                                                                                                                              \
		g_TestsRun++;                                                                                                                            \
		int failedBefore = g_TestsFailed;                                                                                                        \
		ScopedTimer timer;                                                                                                                       \
		printf("  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "   %-50s", #name);                                                                      \
		Test_##name(alloc);                                                                                                                      \
		double elapsedMs = timer.GetElapsedMilliseconds();                                                                                       \
		if (g_TestsFailed == failedBefore)                                                                                                       \
		{                                                                                                                                        \
			g_TestsPassed++;                                                                                                                     \
			printf(ANSI_COLOR_GREEN "PASS" ANSI_COLOR_RESET ANSI_COLOR_DIM "  (%.3fms)" ANSI_COLOR_RESET "\n", elapsedMs);                       \
		}                                                                                                                                        \
		else                                                                                                                                     \
		{                                                                                                                                        \
			printf(ANSI_COLOR_RED "FAIL" ANSI_COLOR_RESET ANSI_COLOR_DIM "  (%.3fms)" ANSI_COLOR_RESET "\n", elapsedMs);                         \
			printf("  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "     " ANSI_COLOR_DIM "at %s:%d" ANSI_COLOR_RESET "\n", g_LastFailure.file,         \
			       g_LastFailure.line);                                                                                                          \
			printf("%s", g_LastFailure.message);                                                                                                 \
			printf("  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "     " ANSI_COLOR_DIM "rerun:" ANSI_COLOR_RESET " %s -filter=%s\n", g_RerunCommand, \
			       #name);                                                                                                                       \
		}                                                                                                                                        \
	}                                                                                                                                            \
	static void Test_##name(Allocator* alloc)

#define WDT_RECORD_FAILURE(...)                                                      \
	do                                                                               \
	{                                                                                \
		g_LastFailure.file = __FILE__;                                               \
		g_LastFailure.line = __LINE__;                                               \
		snprintf(g_LastFailure.message, sizeof(g_LastFailure.message), __VA_ARGS__); \
		g_TestsFailed++;                                                             \
	} while (0)

#define ASSERT_TRUE(expr)                                                                                                                 \
	do                                                                                                                                    \
	{                                                                                                                                     \
		if (!(expr))                                                                                                                      \
		{                                                                                                                                 \
			WDT_RECORD_FAILURE("  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "     " ANSI_COLOR_BOLD "ASSERT_TRUE" ANSI_COLOR_RESET "(%s)\n"   \
			                   "  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "       " ANSI_COLOR_DIM "Expected:" ANSI_COLOR_RESET " true\n"   \
			                   "  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "       " ANSI_COLOR_DIM "Actual:  " ANSI_COLOR_RESET " false\n", \
			                   #expr);                                                                                                    \
			return;                                                                                                                       \
		}                                                                                                                                 \
	} while (0)

#define ASSERT_FALSE(expr)                                                                                                               \
	do                                                                                                                                   \
	{                                                                                                                                    \
		if ((expr))                                                                                                                      \
		{                                                                                                                                \
			WDT_RECORD_FAILURE("  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "     " ANSI_COLOR_BOLD "ASSERT_FALSE" ANSI_COLOR_RESET "(%s)\n" \
			                   "  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "       " ANSI_COLOR_DIM "Expected:" ANSI_COLOR_RESET " false\n" \
			                   "  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "       " ANSI_COLOR_DIM "Actual:  " ANSI_COLOR_RESET " true\n", \
			                   #expr);                                                                                                   \
			return;                                                                                                                      \
		}                                                                                                                                \
	} while (0)

#define ASSERT_EQ(actual, expected)                                                                                                       \
	do                                                                                                                                    \
	{                                                                                                                                     \
		auto actualValue   = (actual);                                                                                                    \
		auto expectedValue = (expected);                                                                                                  \
		if (actualValue != expectedValue)                                                                                                 \
		{                                                                                                                                 \
			WDT_RECORD_FAILURE("  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "     " ANSI_COLOR_BOLD "ASSERT_EQ" ANSI_COLOR_RESET "(%s, %s)\n" \
			                   "  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "       " ANSI_COLOR_DIM "Expected:" ANSI_COLOR_RESET " %lld\n"   \
			                   "  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "       " ANSI_COLOR_DIM "Actual:  " ANSI_COLOR_RESET " %lld\n",  \
			                   #actual, #expected, (long long)expectedValue, (long long)actualValue);                                     \
			return;                                                                                                                       \
		}                                                                                                                                 \
	} while (0)

#define ASSERT_STR_EQ(actual, expected)                                                                                                       \
	do                                                                                                                                        \
	{                                                                                                                                         \
		StringView actualValue    = (actual);                                                                                                 \
		const char* expectedValue = (expected);                                                                                               \
		u64 expectedValueLength   = strlen(expectedValue);                                                                                    \
		if (actualValue.Length() != expectedValueLength || memcmp(actualValue.Data(), expectedValue, expectedValueLength) != 0)               \
		{                                                                                                                                     \
			WDT_RECORD_FAILURE("  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "     " ANSI_COLOR_BOLD "ASSERT_STR_EQ" ANSI_COLOR_RESET "(%s, %s)\n" \
			                   "  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "       " ANSI_COLOR_DIM "Expected:" ANSI_COLOR_RESET                 \
			                   " \"%s\" " ANSI_COLOR_DIM "(length %llu)" ANSI_COLOR_RESET "\n"                                                \
			                   "  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "       " ANSI_COLOR_DIM "Actual:  " ANSI_COLOR_RESET                 \
			                   " \"%.*s\" " ANSI_COLOR_DIM "(length %llu)" ANSI_COLOR_RESET "\n",                                             \
			                   #actual, #expected, expectedValue, (unsigned long long)expectedValueLength, (int)actualValue.Length(),         \
			                   actualValue.Data(), (unsigned long long)actualValue.Length());                                                 \
			return;                                                                                                                           \
		}                                                                                                                                     \
	} while (0)

#define ASSERT_NO_DIAGNOSTICS(diag)           \
	do                                        \
	{                                         \
		ASSERT_EQ((diag).ErrorCount(), 0u);   \
		ASSERT_EQ((diag).WarningCount(), 0u); \
	} while (0)

#define ASSERT_STR_CONTAINS(actual, expected)                                                                                                        \
	do                                                                                                                                               \
	{                                                                                                                                                \
		const char* actualValue   = (actual);                                                                                                        \
		const char* expectedValue = (expected);                                                                                                      \
		if (actualValue == NULL || strstr(actualValue, expectedValue) == NULL)                                                                       \
		{                                                                                                                                            \
			WDT_RECORD_FAILURE("  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "     " ANSI_COLOR_BOLD "ASSERT_STR_CONTAINS" ANSI_COLOR_RESET "(%s, %s)\n"  \
			                   "  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "       " ANSI_COLOR_DIM "Expected substring:" ANSI_COLOR_RESET " \"%s\"\n"  \
			                   "  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "       " ANSI_COLOR_DIM "In string:         " ANSI_COLOR_RESET " \"%s\"\n", \
			                   #actual, #expected, expectedValue, actualValue ? actualValue : "(null)");                                             \
			return;                                                                                                                                  \
		}                                                                                                                                            \
	} while (0)

#define RUN_TEST(name) RunTest_##name(&arena)

	inline File MakeTestFile(Allocator* alloc, const char* source)
	{
		return File{alloc, String::FromCStr(alloc, source), String::FromCStr(alloc, "test.wdt")};
	}

	inline void PrintSection(const char* name)
	{
		printf("  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "\n");
		printf("  " ANSI_COLOR_DIM "+--" ANSI_COLOR_RESET " " ANSI_COLOR_CYAN ANSI_COLOR_BOLD "%s" ANSI_COLOR_RESET "\n", name);
	}

	inline TestResults PrintTestSummary(const char* suiteName, f64 totalMs)
	{
		TestResults results;
		results.run    = g_TestsRun;
		results.passed = g_TestsPassed;
		results.failed = g_TestsFailed;
		results.timeMs = totalMs;

		printf("  " ANSI_COLOR_DIM "|" ANSI_COLOR_RESET "\n");
		printf("  " ANSI_COLOR_DIM "`--" ANSI_COLOR_RESET " ");
		if (g_TestsFailed == 0)
		{
			printf(ANSI_COLOR_GREEN ANSI_COLOR_BOLD "All %d %s tests passed" ANSI_COLOR_RESET ANSI_COLOR_DIM "  (%.2fms total)" ANSI_COLOR_RESET "\n",
			       g_TestsRun, suiteName, totalMs);
		}
		else
		{
			printf(ANSI_COLOR_RED ANSI_COLOR_BOLD "%d of %d %s tests failed" ANSI_COLOR_RESET ANSI_COLOR_DIM "  (%.2fms total)" ANSI_COLOR_RESET "\n",
			       g_TestsFailed, g_TestsRun, suiteName, totalMs);
			printf("      " ANSI_COLOR_GREEN "%d passed" ANSI_COLOR_RESET ", " ANSI_COLOR_RED "%d failed" ANSI_COLOR_RESET "\n", g_TestsPassed,
			       g_TestsFailed);
		}
		return results;
	}

} // namespace Wandelt
