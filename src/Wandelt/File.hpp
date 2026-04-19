#pragma once

#include "Wandelt/Defines.hpp"
#include "Wandelt/Memory.hpp"
#include "Wandelt/String.hpp"

namespace Wandelt
{

	inline constexpr u32 SourceDisplayTabWidth = 4;

	inline u32 AdvanceDisplayOffset(u32 offset, char c)
	{
		if (c == '\t')
			return offset + (SourceDisplayTabWidth - (offset % SourceDisplayTabWidth));

		return offset + 1;
	}

	struct FileLocation
	{
		u32 row;
		u32 col;
	};

	class File
	{
	public:
		File(Allocator* alloc, String path);
		File(Allocator* alloc, String source, String name);
		~File() = default;

		NONCOPYABLE(File);
		NONMOVABLE(File);

	public:
		void PrintInfo() const;
		FileLocation ResolveLocation(u32 offset) const;

		const String& Path() const { return m_Path; }
		const String& Name() const { return m_Name; }
		const String& Content() const { return m_Content; }

		StringView GetViewOverPartOfContent(u32 offset, u32 length) const;

	private:
		Allocator* m_Allocator = nullptr;
		String m_Path;    // Path to the file (relative to the project root)
		String m_Name;    // Name of the file (with extension)
		String m_Content; // Content of the file
	};

	bool FileExists(const String& path);

} // namespace Wandelt
