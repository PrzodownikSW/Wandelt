#include "String.hpp"

#include <cstring>

namespace Wandelt
{

	String String::FromCStr(Allocator* alloc, const char* str)
	{
		String s;
		s.m_Allocator = alloc;
		s.m_Len       = strlen(str);
		s.m_Data      = (char*)alloc->Alloc(s.m_Len + 1);
		memcpy(s.m_Data, str, s.m_Len + 1);

		return s;
	}

	String String::WithCapacity(Allocator* alloc, u64 len)
	{
		String s;
		s.m_Allocator = alloc;
		s.m_Len       = len;
		s.m_Data      = (char*)alloc->Alloc(len + 1);
		s.m_Data[len] = '\0';

		return s;
	}

	String::~String()
	{
		if (m_Allocator && m_Data)
			m_Allocator->Free(m_Data, m_Len + 1);
	}

	String::String(String&& other) noexcept : m_Allocator(other.m_Allocator), m_Data(other.m_Data), m_Len(other.m_Len)
	{
		other.m_Allocator = nullptr;
		other.m_Data      = nullptr;
		other.m_Len       = 0;
	}

	String& String::operator=(String&& other) noexcept
	{
		if (this != &other)
		{
			if (m_Allocator && m_Data)
				m_Allocator->Free(m_Data, m_Len + 1);

			m_Allocator = other.m_Allocator;
			m_Data      = other.m_Data;
			m_Len       = other.m_Len;

			other.m_Allocator = nullptr;
			other.m_Data      = nullptr;
			other.m_Len       = 0;
		}

		return *this;
	}

	StringView StringView::FromCStr(const char* str)
	{
		return StringView{str, strlen(str)};
	}

	bool StringView::operator==(std::string_view other) const
	{
		if (m_Len != other.size())
			return false;

		return memcmp(m_Data, other.data(), m_Len) == 0;
	}

} // namespace Wandelt
