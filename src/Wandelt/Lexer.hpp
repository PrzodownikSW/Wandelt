#pragma once

#include "Wandelt/Diagnostics.hpp"
#include "Wandelt/File.hpp"
#include "Wandelt/Token.hpp"

namespace Wandelt
{

	class Lexer
	{
	public:
		Lexer(File* file, Diagnostics* diagnostics);
		~Lexer();

		NONCOPYABLE(Lexer);
		NONMOVABLE(Lexer);

	public:
		void EatToken();
		Token PeekToken();
		Token PeekTokenAtOffset(i32 offset);
		void DebugPrintToken(Token token);

	private:
		void Advance();
		void SkipWhitespace();
		Token CreateNewToken(TokenType type);
		Token LexIdentifierOrKeyword();
		Token LexDigit(char firstChar);
		Token LexToken();

	private:
		File* m_File               = nullptr;
		Diagnostics* m_Diagnostics = nullptr;
		Token m_CachedToken;
		const char* m_CurrentChar = nullptr; // The current character being lexed
		u32 m_LexingStartOffset   = 0;
	};

} // namespace Wandelt
