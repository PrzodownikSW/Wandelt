#include "Diagnostics.hpp"

#include <charconv>
#include <cstdio>
#include <cstring>
#include <string>

#include "Defines.hpp"
#include "File.hpp"
#include "Platform.hpp"

namespace Wandelt
{

	template <typename Integer>
	static void AppendInteger(std::string& out, Integer value)
	{
		char buffer[32];
		auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
		ASSERT(ec == std::errc{});
		out.append(buffer, ptr);
	}

	static u32 MeasureDisplayWidth(std::string_view text)
	{
		u32 width = 0;
		for (char c : text) width = AdvanceDisplayOffset(width, c);

		return width;
	}

	static void AppendSpaces(std::string& out, u64 count)
	{
		out.append((size_t)count, ' ');
	}

	static void AppendExpandedRange(std::string& out, std::string_view text, u64 displayStart, u64 displayEnd)
	{
		u64 width = 0;
		for (char c : text)
		{
			u64 nextWidth = AdvanceDisplayOffset((u32)width, c);
			if (nextWidth <= displayStart)
			{
				width = nextWidth;
				continue;
			}

			if (width >= displayEnd)
				break;

			if (c == '\t')
			{
				u64 padStart = width > displayStart ? width : displayStart;
				u64 padEnd   = nextWidth < displayEnd ? nextWidth : displayEnd;
				AppendSpaces(out, padEnd - padStart);
				width = nextWidth;
				continue;
			}

			out.push_back(c);
			width = nextWidth;
		}
	}

	void Diagnostics::PrintAtLocation(Span span, const File* file, std::string_view message, Severity severity)
	{
		std::string formatted = FormatAtLocation(span, file, message, severity, Platform::GetTerminalWidth(), m_UseColor);
		fputs(formatted.c_str(), stdout);
	}

	std::string Diagnostics::FormatAtLocation(Span span, const File* file, std::string_view message, Severity severity, int termWidth,
	                                          bool useColor) const
	{
		const char* severityStr = "";
		const char* colorCode   = "";
		switch (severity)
		{
		case Severity::Note:
			severityStr = "note";
			colorCode   = ANSI_COLOR_BLUE;
			break;
		case Severity::Warning:
			severityStr = "warning";
			colorCode   = ANSI_COLOR_YELLOW;
			break;
		case Severity::Error:
			severityStr = "error";
			colorCode   = ANSI_COLOR_RED;
			break;
		}

		const char* bold  = useColor ? ANSI_COLOR_BOLD : "";
		const char* white = useColor ? ANSI_COLOR_WHITE : "";
		const char* reset = useColor ? ANSI_COLOR_RESET : "";
		if (!useColor)
			colorCode = "";

		FileLocation loc = file->ResolveLocation(span.begin);

		u64 reserveEstimate = 128 + message.size();

		const char* src = file->Content().Data();
		u64 srcLen      = file->Content().Length();
		u64 lineStart   = span.begin;
		u64 lineEnd     = span.begin;

		while (lineStart > 0 && src[lineStart - 1] != '\n') lineStart--;
		while (lineEnd < srcLen && src[lineEnd] != '\n') lineEnd++;

		reserveEstimate += (lineEnd - lineStart) * SourceDisplayTabWidth;

		u64 prevStart           = 0;
		u64 prevEnd             = 0;
		bool hasPrevLineContext = lineStart == lineEnd && lineStart > 0;
		if (hasPrevLineContext)
		{
			prevEnd   = lineStart - 1;
			prevStart = prevEnd;
			while (prevStart > 0 && src[prevStart - 1] != '\n') prevStart--;
			reserveEstimate += (prevEnd - prevStart) * SourceDisplayTabWidth;
		}

		std::string result;
		result.reserve((size_t)reserveEstimate);
		result += bold;
		result += white;
		result += file->Name().Data();
		result.push_back(':');
		AppendInteger(result, loc.row);
		result.push_back(':');
		AppendInteger(result, loc.col);
		result += ": ";
		result += colorCode;
		result += severityStr;
		result += ":";
		result += reset;
		result.push_back(' ');
		result += bold;
		result.append(message.data(), message.size());
		result += reset;
		result.push_back('\n');

		int gutterWidth = snprintf(NULL, 0, "%u", loc.row);

		if (hasPrevLineContext)
		{
			result.push_back(' ');
			for (int i = 0; i < gutterWidth - 1; i++) result.push_back(' ');
			AppendInteger(result, loc.row - 1);
			result += " | ";
			AppendExpandedRange(result, std::string_view{src + prevStart, prevEnd - prevStart}, 0, UINT64_MAX);
			result.push_back('\n');
		}

		std::string_view lineText = std::string_view{src + lineStart, lineEnd - lineStart};
		u64 colOffset             = MeasureDisplayWidth(std::string_view{src + lineStart, span.begin - lineStart});
		u64 spanEnd               = span.end;
		if (spanEnd > lineEnd)
			spanEnd = lineEnd;

		u64 spanLen = MeasureDisplayWidth(std::string_view{src + span.begin, spanEnd - span.begin});
		if (spanLen == 0)
			spanLen = 1;

		u64 lineLen     = MeasureDisplayWidth(lineText);
		int gutterChars = gutterWidth + 4;
		int avail       = termWidth - gutterChars;
		if (avail < 20)
			avail = 20;

		u64 viewStart  = 0;
		u64 viewEnd    = lineLen;
		bool clipLeft  = false;
		bool clipRight = false;

		if ((int)lineLen > avail)
		{
			int margin = (avail - (int)spanLen) / 2;
			if (margin < 8)
				margin = 8;

			i64 desiredStart = (i64)colOffset - margin;
			if (desiredStart < 0)
				desiredStart = 0;

			viewStart = (u64)desiredStart;
			viewEnd   = viewStart + (u64)avail;

			if (viewStart > 0)
			{
				viewStart += 3;
				clipLeft = true;
			}
			if (viewEnd < lineLen)
			{
				viewEnd -= 3;
				clipRight = true;
			}
			if (viewEnd > lineLen)
				viewEnd = lineLen;
		}

		result.push_back(' ');
		AppendInteger(result, loc.row);
		result += " | ";
		if (clipLeft)
			result += "...";
		AppendExpandedRange(result, lineText, viewStart, viewEnd);
		if (clipRight)
			result += "...";
		result.push_back('\n');

		u64 displayOffset = colOffset - viewStart;

		result.push_back(' ');
		for (int i = 0; i < gutterWidth; i++) result.push_back(' ');
		result += " | ";
		if (clipLeft)
			result += "   ";
		result += colorCode;
		result += bold;

		AppendSpaces(result, displayOffset);

		result.push_back('^');

		for (u64 i = 1; i < spanLen && (displayOffset + i) < (viewEnd - viewStart); i++) result.push_back('~');

		result += reset;
		result.push_back('\n');

		return result;
	}

	void Diagnostics::EmitFormatted(Severity severity, Span span, const File* file, std::string_view message)
	{
		switch (severity)
		{
		case Severity::Note:
			break;
		case Severity::Warning:
			m_WarningCount++;
			break;
		case Severity::Error:
			m_ErrorCount++;
			break;
		}

		if (m_CaptureEnabled)
		{
			if (m_CapturedCount < MaxCaptured)
			{
				Entry* entry     = &m_Captured[m_CapturedCount++];
				entry->severity  = severity;
				FileLocation loc = file->ResolveLocation(span.begin);
				entry->line      = loc.row;
				entry->col       = loc.col;

				u64 copyLen = message.size();
				if (copyLen >= sizeof(entry->message))
					copyLen = sizeof(entry->message) - 1;
				memcpy(entry->message, message.data(), copyLen);
				entry->message[copyLen] = '\0';
			}
		}
		else
		{
			PrintAtLocation(span, file, message, severity);
		}
	}

	void Diagnostics::Reset()
	{
		m_ErrorCount   = 0;
		m_WarningCount = 0;
	}

	void Diagnostics::EnableCapture()
	{
		m_CaptureEnabled = true;
		m_CapturedCount  = 0;
	}

	void Diagnostics::DisableCapture()
	{
		m_CaptureEnabled = false;
	}

	Diagnostics::Entry* Diagnostics::GetCaptured(u32 index)
	{
		ASSERT(index < m_CapturedCount);
		return &m_Captured[index];
	}

	Diagnostics::CaptureScope::CaptureScope(Diagnostics& diag) : m_Diagnostics(diag)
	{
		m_Diagnostics.Reset();
		m_Diagnostics.EnableCapture();
	}

	Diagnostics::CaptureScope::~CaptureScope()
	{
		m_Diagnostics.DisableCapture();
		m_Diagnostics.Reset();
	}

} // namespace Wandelt
