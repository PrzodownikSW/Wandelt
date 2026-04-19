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
	inline bool g_TestUseColor            = true;

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

	inline void SetTestUseColor(bool useColor)
	{
		g_TestUseColor = useColor;
	}

	inline const char* TestColor(const char* color)
	{
		return g_TestUseColor ? color : "";
	}

	inline bool TestMatchesFilter(const char* testName)
	{
		if (g_TestFilter[0] == '\0')
			return true;
		return strcmp(g_TestFilter, testName) == 0;
	}

#define TEST(name)                                                                                                                              \
	static void Test_##name(Allocator* alloc);                                                                                                  \
	static void RunTest_##name(Allocator* alloc)                                                                                                \
	{                                                                                                                                           \
		if (!TestMatchesFilter(#name))                                                                                                          \
			return;                                                                                                                             \
		g_TestsRun++;                                                                                                                           \
		int failedBefore = g_TestsFailed;                                                                                                       \
		ScopedTimer timer;                                                                                                                      \
		printf("  %s|%s   %-70s", TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), #name);                                               \
		Test_##name(alloc);                                                                                                                     \
		double elapsedMs = timer.GetElapsedMilliseconds();                                                                                      \
		if (g_TestsFailed == failedBefore)                                                                                                      \
		{                                                                                                                                       \
			g_TestsPassed++;                                                                                                                    \
			printf("%sPASS%s%s  (%.3fms)%s\n", TestColor(ANSI_COLOR_GREEN), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM), elapsedMs,  \
			       TestColor(ANSI_COLOR_RESET));                                                                                                \
		}                                                                                                                                       \
		else                                                                                                                                    \
		{                                                                                                                                       \
			printf("%sFAIL%s%s  (%.3fms)%s\n", TestColor(ANSI_COLOR_RED), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM), elapsedMs,    \
			       TestColor(ANSI_COLOR_RESET));                                                                                                \
			printf("  %s|%s     %sat %s:%d%s\n", TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM),             \
			       g_LastFailure.file, g_LastFailure.line, TestColor(ANSI_COLOR_RESET));                                                        \
			printf("%s", g_LastFailure.message);                                                                                                \
			printf("  %s|%s     %srerun:%s %s -filter=%s\n", TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM), \
			       TestColor(ANSI_COLOR_RESET), g_RerunCommand, #name);                                                                         \
		}                                                                                                                                       \
	}                                                                                                                                           \
	static void Test_##name(Allocator* alloc)

#define WDT_RECORD_FAILURE(...)                                                      \
	do                                                                               \
	{                                                                                \
		g_LastFailure.file = __FILE__;                                               \
		g_LastFailure.line = __LINE__;                                               \
		snprintf(g_LastFailure.message, sizeof(g_LastFailure.message), __VA_ARGS__); \
		g_TestsFailed++;                                                             \
	} while (0)

#define ASSERT_TRUE(expr)                                                                                                                       \
	do                                                                                                                                          \
	{                                                                                                                                           \
		if (!(expr))                                                                                                                            \
		{                                                                                                                                       \
			WDT_RECORD_FAILURE("  %s|%s     %sASSERT_TRUE%s(%s)\n"                                                                              \
			                   "  %s|%s       %sExpected:%s true\n"                                                                             \
			                   "  %s|%s       %sActual:  %s false\n",                                                                           \
			                   TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_BOLD), TestColor(ANSI_COLOR_RESET), \
			                   #expr, TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM),                        \
			                   TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM),  \
			                   TestColor(ANSI_COLOR_RESET));                                                                                    \
			return;                                                                                                                             \
		}                                                                                                                                       \
	} while (0)

#define ASSERT_FALSE(expr)                                                                                                                      \
	do                                                                                                                                          \
	{                                                                                                                                           \
		if ((expr))                                                                                                                             \
		{                                                                                                                                       \
			WDT_RECORD_FAILURE("  %s|%s     %sASSERT_FALSE%s(%s)\n"                                                                             \
			                   "  %s|%s       %sExpected:%s false\n"                                                                            \
			                   "  %s|%s       %sActual:  %s true\n",                                                                            \
			                   TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_BOLD), TestColor(ANSI_COLOR_RESET), \
			                   #expr, TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM),                        \
			                   TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM),  \
			                   TestColor(ANSI_COLOR_RESET));                                                                                    \
			return;                                                                                                                             \
		}                                                                                                                                       \
	} while (0)

#define ASSERT_EQ(actual, expected)                                                                                                             \
	do                                                                                                                                          \
	{                                                                                                                                           \
		auto actualValue   = (actual);                                                                                                          \
		auto expectedValue = (expected);                                                                                                        \
		if (actualValue != expectedValue)                                                                                                       \
		{                                                                                                                                       \
			WDT_RECORD_FAILURE("  %s|%s     %sASSERT_EQ%s(%s, %s)\n"                                                                            \
			                   "  %s|%s       %sExpected:%s %lld\n"                                                                             \
			                   "  %s|%s       %sActual:  %s %lld\n",                                                                            \
			                   TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_BOLD), TestColor(ANSI_COLOR_RESET), \
			                   #actual, #expected, TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM),           \
			                   TestColor(ANSI_COLOR_RESET), (long long)expectedValue, TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET),   \
			                   TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), (long long)actualValue);                                 \
			return;                                                                                                                             \
		}                                                                                                                                       \
	} while (0)

#define ASSERT_STR_EQ(actual, expected)                                                                                                         \
	do                                                                                                                                          \
	{                                                                                                                                           \
		StringView actualValue    = (actual);                                                                                                   \
		const char* expectedValue = (expected);                                                                                                 \
		u64 expectedValueLength   = strlen(expectedValue);                                                                                      \
		if (actualValue.Length() != expectedValueLength || memcmp(actualValue.Data(), expectedValue, expectedValueLength) != 0)                 \
		{                                                                                                                                       \
			WDT_RECORD_FAILURE("  %s|%s     %sASSERT_STR_EQ%s(%s, %s)\n"                                                                        \
			                   "  %s|%s       %sExpected:%s \"%s\" %s(length %llu)%s\n"                                                         \
			                   "  %s|%s       %sActual:  %s \"%.*s\" %s(length %llu)%s\n",                                                      \
			                   TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_BOLD), TestColor(ANSI_COLOR_RESET), \
			                   #actual, #expected, TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM),           \
			                   TestColor(ANSI_COLOR_RESET), expectedValue, TestColor(ANSI_COLOR_DIM), (unsigned long long)expectedValueLength,  \
			                   TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM),  \
			                   TestColor(ANSI_COLOR_RESET), (int)actualValue.Length(), actualValue.Data(), TestColor(ANSI_COLOR_DIM),           \
			                   (unsigned long long)actualValue.Length(), TestColor(ANSI_COLOR_RESET));                                          \
			return;                                                                                                                             \
		}                                                                                                                                       \
	} while (0)

#define ASSERT_NO_DIAGNOSTICS(diag)           \
	do                                        \
	{                                         \
		ASSERT_EQ((diag).ErrorCount(), 0u);   \
		ASSERT_EQ((diag).WarningCount(), 0u); \
	} while (0)

#define ASSERT_STR_CONTAINS(actual, expected)                                                                                                   \
	do                                                                                                                                          \
	{                                                                                                                                           \
		const char* actualValue   = (actual);                                                                                                   \
		const char* expectedValue = (expected);                                                                                                 \
		if (actualValue == NULL || strstr(actualValue, expectedValue) == NULL)                                                                  \
		{                                                                                                                                       \
			WDT_RECORD_FAILURE("  %s|%s     %sASSERT_STR_CONTAINS%s(%s, %s)\n"                                                                  \
			                   "  %s|%s       %sExpected substring:%s \"%s\"\n"                                                                 \
			                   "  %s|%s       %sIn string:         %s \"%s\"\n",                                                                \
			                   TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_BOLD), TestColor(ANSI_COLOR_RESET), \
			                   #actual, #expected, TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM),           \
			                   TestColor(ANSI_COLOR_RESET), expectedValue, TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET),              \
			                   TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), actualValue ? actualValue : "(null)");                   \
			return;                                                                                                                             \
		}                                                                                                                                       \
	} while (0)

#define RUN_TEST(name) RunTest_##name(&arena)

	inline File MakeTestFile(Allocator* alloc, const char* source)
	{
		return File{alloc, String::FromCStr(alloc, source), String::FromCStr(alloc, "test.wdt")};
	}

	inline void PrintSection(const char* name)
	{
		printf("  %s|%s\n", TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET));
		printf("  %s+--%s %s%s%s%s\n", TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_CYAN), TestColor(ANSI_COLOR_BOLD),
		       name, TestColor(ANSI_COLOR_RESET));
	}

	inline TestResults PrintTestSummary(const char* suiteName, f64 totalMs)
	{
		TestResults results;
		results.run    = g_TestsRun;
		results.passed = g_TestsPassed;
		results.failed = g_TestsFailed;
		results.timeMs = totalMs;

		printf("  %s|%s\n", TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET));
		printf("  %s`--%s ", TestColor(ANSI_COLOR_DIM), TestColor(ANSI_COLOR_RESET));
		if (g_TestsFailed == 0)
		{
			printf("%s%sAll %d %s tests passed%s%s  (%.2fms total)%s\n", TestColor(ANSI_COLOR_GREEN), TestColor(ANSI_COLOR_BOLD), g_TestsRun,
			       suiteName, TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM), totalMs, TestColor(ANSI_COLOR_RESET));
		}
		else
		{
			printf("%s%s%d of %d %s tests failed%s%s  (%.2fms total)%s\n", TestColor(ANSI_COLOR_RED), TestColor(ANSI_COLOR_BOLD), g_TestsFailed,
			       g_TestsRun, suiteName, TestColor(ANSI_COLOR_RESET), TestColor(ANSI_COLOR_DIM), totalMs, TestColor(ANSI_COLOR_RESET));
			printf("      %s%d passed%s, %s%d failed%s\n", TestColor(ANSI_COLOR_GREEN), g_TestsPassed, TestColor(ANSI_COLOR_RESET),
			       TestColor(ANSI_COLOR_RED), g_TestsFailed, TestColor(ANSI_COLOR_RESET));
		}
		return results;
	}

} // namespace Wandelt
