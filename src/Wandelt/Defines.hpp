#pragma once

#include <cstddef>

// Unsigned int types.
using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

// Signed int types.
using i8  = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long long;

// Floating point types
using f32 = float;
using f64 = double;

// Other types.
using usize = std::size_t;
using isize = std::ptrdiff_t;

static_assert(sizeof(u8) == 1, "u8 must be 1 byte");
static_assert(sizeof(u16) == 2, "u16 must be 2 bytes");
static_assert(sizeof(u32) == 4, "u32 must be 4 bytes");
static_assert(sizeof(u64) == 8, "u64 must be 8 bytes");

static_assert(sizeof(i8) == 1, "i8 must be 1 byte");
static_assert(sizeof(i16) == 2, "i16 must be 2 bytes");
static_assert(sizeof(i32) == 4, "i32 must be 4 bytes");
static_assert(sizeof(i64) == 8, "i64 must be 8 bytes");

static_assert(sizeof(f32) == 4, "f32 must be 4 bytes");
static_assert(sizeof(f64) == 8, "f64 must be 8 bytes");

#define BIT(x) (1ULL << (x))

#define ANSI_COLOR_BLACK      "\x1b[30m"
#define ANSI_COLOR_RED        "\x1b[31m"
#define ANSI_COLOR_GREEN      "\x1b[32m"
#define ANSI_COLOR_YELLOW     "\x1b[33m"
#define ANSI_COLOR_ORANGE     "\x1b[38;5;208m"
#define ANSI_COLOR_BLUE       "\x1b[34m"
#define ANSI_COLOR_MAGENTA    "\x1b[35m"
#define ANSI_COLOR_CYAN       "\x1b[36m"
#define ANSI_COLOR_WHITE      "\x1b[37m"
#define ANSI_BG_COLOR_BLACK   "\x1b[40m"
#define ANSI_BG_COLOR_RED     "\x1b[41m"
#define ANSI_BG_COLOR_GREEN   "\x1b[42m"
#define ANSI_BG_COLOR_YELLOW  "\x1b[43m"
#define ANSI_BG_COLOR_BLUE    "\x1b[44m"
#define ANSI_BG_COLOR_MAGENTA "\x1b[45m"
#define ANSI_BG_COLOR_CYAN    "\x1b[46m"
#define ANSI_BG_COLOR_WHITE   "\x1b[47m"
#define ANSI_COLOR_BOLD       "\x1b[1m"
#define ANSI_COLOR_DIM        "\x1b[2m"
#define ANSI_COLOR_RESET      "\x1b[0m"

#define NONCOPYABLE(T)               \
	T(const T&)            = delete; \
	T& operator=(const T&) = delete
#define NONMOVABLE(T)           \
	T(T&&)            = delete; \
	T& operator=(T&&) = delete

#define NODISCARD [[nodiscard]]
#define UNUSED(x) (void)(x)

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x)      STRINGIFY_IMPL(x)

#if defined(__clang__)
	#define WDT_COMPILER_NAME  "clang"
	#define WDT_COMPILER_CLANG (1)
	#define WDT_COMPILER_MSVC  (0)
	#define WDT_COMPILER_GCC   (0)
#elif defined(__GNUC__)
	#define WDT_COMPILER_NAME  "gcc"
	#define WDT_COMPILER_CLANG (0)
	#define WDT_COMPILER_MSVC  (0)
	#define WDT_COMPILER_GCC   (1)
#elif defined(_MSC_VER)
	#define WDT_COMPILER_NAME  "msvc"
	#define WDT_COMPILER_CLANG (0)
	#define WDT_COMPILER_MSVC  (1)
	#define WDT_COMPILER_GCC   (0)
#else
static_assert(false, "Unsupported compiler!");
#endif

#if WDT_COMPILER_MSVC
	#define DEBUG_BREAK() __debugbreak()
#elif WDT_COMPILER_CLANG || WDT_COMPILER_GCC
	#if defined(__i386__) || defined(__x86_64__)
		#define DEBUG_BREAK() __asm__ volatile("int3")
	#elif defined(__aarch64__) || defined(__arm__)
		#define DEBUG_BREAK() __builtin_debugtrap()
	#else
		#define DEBUG_BREAK() __builtin_trap()
	#endif
#endif

#if WDT_COMPILER_MSVC
	#define INLINE   __forceinline
	#define NOINLINE __declspec(noinline)
#elif WDT_COMPILER_CLANG || WDT_COMPILER_GCC
	#define INLINE   inline __attribute__((always_inline))
	#define NOINLINE __attribute__((noinline))
#else
static_assert(false, "Unsupported compiler!");
#endif

INLINE constexpr u64 Kilobytes(u64 x)
{
	return x * 1024;
}

INLINE constexpr u64 Megabytes(u64 x)
{
	return Kilobytes(x) * 1024;
}

INLINE constexpr u64 Gigabytes(u64 x)
{
	return Megabytes(x) * 1024;
}

template <typename T, usize N>
constexpr usize ArraySize(const T (&)[N])
{
	return N;
}

#if defined(_WIN32)
	#define WDT_OS_NAME    "windows"
	#define WDT_OS_WINDOWS (1)
	#define WDT_OS_LINUX   (0)
	#define WDT_OS_MACOS   (0)
#elif defined(__linux__)
	#define WDT_OS_NAME    "linux"
	#define WDT_OS_WINDOWS (0)
	#define WDT_OS_LINUX   (1)
	#define WDT_OS_MACOS   (0)
#elif defined(__APPLE__)
	#define WDT_OS_NAME    "macos"
	#define WDT_OS_WINDOWS (0)
	#define WDT_OS_LINUX   (0)
	#define WDT_OS_MACOS   (1)
#else
static_assert(false, "Unsupported OS!");
#endif

#if WDT_COMPILER_CLANG || WDT_COMPILER_GCC
	#if defined(__x86_64__)
		#define WDT_ARCHITECTURE_NAME  "x64"
		#define WDT_ARCHITECTURE_64BIT (true)
		#define WDT_ARCHITECTURE_X64   (1)
		#define WDT_ARCHITECTURE_ARM64 (0)
	#elif defined(__aarch64__)
		#define WDT_ARCHITECTURE_NAME  "arm64"
		#define WDT_ARCHITECTURE_64BIT (true)
		#define WDT_ARCHITECTURE_X64   (0)
		#define WDT_ARCHITECTURE_ARM64 (1)
	#else
static_assert(false, "Unsupported architecture!");
	#endif
#elif WDT_COMPILER_MSVC
	#if defined(_M_X64)
		#define WDT_ARCHITECTURE_NAME  "x64"
		#define WDT_ARCHITECTURE_64BIT (true)
		#define WDT_ARCHITECTURE_X64   (1)
		#define WDT_ARCHITECTURE_ARM64 (0)
	#else
static_assert(false, "Unsupported architecture!");
	#endif
#endif

namespace Wandelt
{
	template <typename F>
	class Defer
	{
	public:
		Defer(Defer&&)                 = delete;
		Defer(const Defer&)            = delete;
		Defer& operator=(const Defer&) = delete;
		Defer& operator=(Defer&&)      = delete;

		template <typename FF>
		Defer(FF&& f) noexcept : cleanup_function(static_cast<FF&&>(f))
		{
		}

		inline ~Defer() noexcept { cleanup_function(); }

	private:
		F cleanup_function;
	};

	template <typename F>
	Defer<F> DeferFunction(F&& f) noexcept
	{
		return {static_cast<F&&>(f)};
	}

#define DEFER_ACTUALLY_JOIN(x, y) x##y
#define DEFER_JOIN(x, y)          DEFER_ACTUALLY_JOIN(x, y)
#define DEFER_UNIQUE_VARNAME(x)   DEFER_JOIN(x, __LINE__)
#define defer(expr)               [[maybe_unused]] auto DEFER_UNIQUE_VARNAME(defer_object) = Wandelt::DeferFunction([&]() { expr; })

} // namespace Wandelt

#if WDT_COMPILER_MSVC
	#define WDT_UNREACHABLE_INTRINSIC() __assume(false)
#elif WDT_COMPILER_CLANG || WDT_COMPILER_GCC
	#define WDT_UNREACHABLE_INTRINSIC() __builtin_unreachable()
#else
static_assert(false, "Unsupported compiler!");
#endif

#define ASSERT_NO_MSG(condition)                                                                             \
	do                                                                                                       \
	{                                                                                                        \
		if (!(condition))                                                                                    \
		{                                                                                                    \
			fprintf(stderr,                                                                                  \
			        ANSI_COLOR_MAGENTA "Assertion failed: %s\n"                                              \
			                           "  File: %s\n"                                                        \
			                           "  Line: %d\n"                                                        \
			                           "\n"                                                                  \
			                           "The compiler encountered an unexpected error!\n"                     \
			                           "Please consider filing an issue on GitHub, including the possibly\n" \
			                           "erroneous source code and this error message, to help to diagnose\n" \
			                           "and fix the issue.\n" ANSI_COLOR_RESET,                              \
			        #condition, __FILE__, __LINE__);                                                         \
			abort();                                                                                         \
		}                                                                                                    \
	} while (0)

#define ASSERT_MSG(condition, ...)                                                                \
	do                                                                                            \
	{                                                                                             \
		if (!(condition))                                                                         \
		{                                                                                         \
			fprintf(stderr,                                                                       \
			        ANSI_COLOR_MAGENTA "Assertion failed: %s\n"                                   \
			                           "  File: %s\n"                                             \
			                           "  Line: %d\n"                                             \
			                           "  Message: ",                                             \
			        #condition, __FILE__, __LINE__);                                              \
			fprintf(stderr, __VA_ARGS__);                                                         \
			fprintf(stderr, "\n\n"                                                                \
			                "The compiler encountered an unexpected error!\n"                     \
			                "Please consider filing an issue on GitHub, including the possibly\n" \
			                "erroneous source code and this error message, to help to diagnose\n" \
			                "and fix the issue.\n" ANSI_COLOR_RESET);                             \
			abort();                                                                              \
		}                                                                                         \
	} while (0)

#define ASSERT_PICK(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, NAME, ...) NAME
#define ASSERT(...)                                                                                                                      \
	ASSERT_PICK(__VA_ARGS__, ASSERT_MSG, ASSERT_MSG, ASSERT_MSG, ASSERT_MSG, ASSERT_MSG, ASSERT_MSG, ASSERT_MSG, ASSERT_MSG, ASSERT_MSG, \
	            ASSERT_NO_MSG, dummy)(__VA_ARGS__)

#define UNREACHABLE()                               \
	do                                              \
	{                                               \
		ASSERT(false, "Reached unreachable code!"); \
		WDT_UNREACHABLE_INTRINSIC();                \
	} while (0)
