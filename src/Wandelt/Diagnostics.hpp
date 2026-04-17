#pragma once

#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "Wandelt/Defines.hpp"
#include "Wandelt/Token.hpp"

namespace Wandelt
{

	class File;

	class Diagnostics
	{
	public:
		enum class Severity
		{
			Note,
			Warning,
			Error,
		};

		struct Entry
		{
			Severity severity;
			u32 line;
			u32 col;
			char message[256];
		};

		static constexpr u32 MaxCaptured = 64;

		Diagnostics()  = default;
		~Diagnostics() = default;

		NONCOPYABLE(Diagnostics);
		NONMOVABLE(Diagnostics);

		template <typename... Args>
		void ReportNote(Span span, const File* file, std::format_string<Args...> fmt, Args&&... args)
		{
			EmitFormatted(Severity::Note, span, file, std::format(fmt, std::forward<Args>(args)...));
		}

		template <typename... Args>
		void ReportWarning(Span span, const File* file, std::format_string<Args...> fmt, Args&&... args)
		{
			EmitFormatted(Severity::Warning, span, file, std::format(fmt, std::forward<Args>(args)...));
		}

		template <typename... Args>
		void ReportError(Span span, const File* file, std::format_string<Args...> fmt, Args&&... args)
		{
			EmitFormatted(Severity::Error, span, file, std::format(fmt, std::forward<Args>(args)...));
		}

		bool HasErrors() const { return m_ErrorCount > 0; }
		bool HasWarnings() const { return m_WarningCount > 0; }
		u32 ErrorCount() const { return m_ErrorCount; }
		u32 WarningCount() const { return m_WarningCount; }
		void Reset();

		void EnableCapture();
		void DisableCapture();
		u32 CapturedCount() const { return m_CapturedCount; }
		Entry* GetCaptured(u32 index);
		std::string FormatAtLocation(Span span, const File* file, std::string_view message, Severity severity, int termWidth, bool useColor) const;

		class CaptureScope
		{
		public:
			explicit CaptureScope(Diagnostics& diag);
			~CaptureScope();

			NONCOPYABLE(CaptureScope);
			NONMOVABLE(CaptureScope);

		private:
			Diagnostics& m_Diagnostics;
		};

	private:
		void EmitFormatted(Severity severity, Span span, const File* file, std::string_view message);
		void PrintAtLocation(Span span, const File* file, std::string_view message, Severity severity);

		u32 m_ErrorCount      = 0;
		u32 m_WarningCount    = 0;
		bool m_CaptureEnabled = false;
		Entry m_Captured[MaxCaptured]{};
		u32 m_CapturedCount = 0;
	};

} // namespace Wandelt
