#pragma once

#include <format>
#include <string_view>

#include "Wandelt/Defines.hpp"
#include "Wandelt/Memory.hpp"

namespace Wandelt
{

	class StringView
	{
	public:
		StringView() = default;
		StringView(const char* data, u64 len) : m_Data(data), m_Len(len) {}

		static StringView FromCStr(const char* str);

		const char* Data() const { return m_Data; }
		u64 Length() const { return m_Len; }

		operator std::string_view() const { return {m_Data, m_Len}; }
		bool operator==(std::string_view other) const;

	private:
		const char* m_Data = nullptr;
		u64 m_Len          = 0;
	};

	class String
	{
	public:
		static String FromCStr(Allocator* alloc, const char* str);
		static String WithCapacity(Allocator* alloc, u64 len);

		String() = default;
		~String();

		NONCOPYABLE(String);

		String(String&& other) noexcept;
		String& operator=(String&& other) noexcept;

		const char* Data() const { return m_Data; }
		char* Data() { return m_Data; }
		u64 Length() const { return m_Len; }

		StringView View() const { return StringView{m_Data, m_Len}; }

	private:
		Allocator* m_Allocator = nullptr;
		char* m_Data           = nullptr;
		u64 m_Len              = 0;
	};

} // namespace Wandelt

template <>
struct std::formatter<Wandelt::String> : std::formatter<std::string_view>
{
	auto format(const Wandelt::String& s, std::format_context& ctx) const
	{
		return std::formatter<std::string_view>::format(std::string_view{s.Data(), s.Length()}, ctx);
	}
};

template <>
struct std::formatter<Wandelt::StringView> : std::formatter<std::string_view>
{
	auto format(Wandelt::StringView s, std::format_context& ctx) const
	{
		return std::formatter<std::string_view>::format(std::string_view{s.Data(), s.Length()}, ctx);
	}
};
