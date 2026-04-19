#include "File.hpp"

#include <cstdio>
#include <filesystem>

namespace Wandelt
{

	namespace fs = std::filesystem;

	File::File(Allocator* alloc, String path) : m_Allocator(alloc), m_Path(std::move(path))
	{
		const fs::path fs_path(m_Path.Data());
		const std::string filename = fs_path.filename().string();
		m_Name                     = String::FromCStr(alloc, filename.c_str());

		FILE* file = fopen(m_Path.Data(), "rb");
		ASSERT(file, "Failed to open the file at path: %s", m_Path.Data());

		std::error_code ec;
		const u64 file_size = (u64)fs::file_size(fs_path, ec);
		ASSERT(!ec, "Failed to get size of file at path: %s", m_Path.Data());

		m_Content = String::WithCapacity(alloc, file_size);

		const u64 bytes_read = fread(m_Content.Data(), 1, file_size, file);
		ASSERT(bytes_read == file_size, "Failed to read file at path: %s", m_Path.Data());

		fclose(file);
	}

	File::File(Allocator* alloc, String source, String name)
	    : m_Allocator(alloc), m_Path(String::FromCStr(alloc, "<in-memory>")), m_Name(std::move(name)), m_Content(std::move(source))
	{
	}

	void File::PrintInfo() const
	{
		std::printf("File '%s' info:\n", m_Name.Data());
		std::printf("- Path: %s\n", m_Path.Data());
		std::printf("- Size: %llu bytes\n", m_Content.Length());
		std::printf("- Content:\n%s\n", m_Content.Data());
	}

	FileLocation File::ResolveLocation(u32 offset) const
	{
		u32 row = 1;
		u32 col = 1;

		const char* data = m_Content.Data();
		const u64 len    = m_Content.Length();

		for (u32 i = 0; i < offset && i < len; i++)
		{
			if (data[i] == '\r')
			{
				row++;
				col = 1;

				if ((i + 1) < offset && (i + 1) < len && data[i + 1] == '\n')
					i++;
			}
			else if (data[i] == '\n')
			{
				row++;
				col = 1;
			}
			else
			{
				col = AdvanceDisplayOffset(col - 1, data[i]) + 1;
			}
		}

		return FileLocation{.row = row, .col = col};
	}

	StringView File::GetViewOverPartOfContent(u32 offset, u32 length) const
	{
		ASSERT(offset + length <= m_Content.Length(), "Requested content part is out of bounds of the file content");
		return StringView{m_Content.Data() + offset, length};
	}

	bool FileExists(const String& path)
	{
		return fs::exists(path.Data());
	}

} // namespace Wandelt
