#include "Lexer.hpp"

#include "Wandelt/Diagnostics.hpp"
#include "Wandelt/String.hpp"
#include "Wandelt/Token.hpp"

#define GetCurrentChar()             (*m_CurrentChar)
#define GetPreviousChar()            (*(m_CurrentChar - 1))
#define GetNextChar()                (*(m_CurrentChar + 1))
#define IsEOF()                      (*m_CurrentChar == '\0')
#define IsAtNewline()                (*m_CurrentChar == '\n')
#define IsCharacterADigit(c)         ((c) >= '0' && (c) <= '9')
#define IsCharacterAnAlphanumeric(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_')

namespace Wandelt
{

	static Token s_InvalidToken = {.type = TOKEN_TYPE_INVALID};

	Lexer::Lexer(File* file, Diagnostics* diagnostics)
	    : m_File(file), m_Diagnostics(diagnostics), m_CachedToken(s_InvalidToken), m_CurrentChar(file->Content().Data())
	{
	}

	Lexer::~Lexer()
	{
	}

	void Lexer::EatToken()
	{
		m_CachedToken = s_InvalidToken;
	}

	Token Lexer::PeekToken()
	{
		if (m_CachedToken.type != TOKEN_TYPE_INVALID)
			return m_CachedToken;

		m_CachedToken = LexToken();

		return m_CachedToken;
	}

	Token Lexer::PeekTokenAtOffset(i32 offset)
	{
		if (offset == 0)
			return PeekToken();

		Token result = PeekToken();

		const char* savedCurrentChar     = m_CurrentChar;
		const u32 savedLexingStartOffset = m_LexingStartOffset;
		const Token savedCachedToken     = m_CachedToken;

		for (i32 i = 0; i < offset; i++)
		{
			EatToken();
			result = PeekToken();
		}

		m_CurrentChar       = savedCurrentChar;
		m_LexingStartOffset = savedLexingStartOffset;
		m_CachedToken       = savedCachedToken;

		return result;
	}

	void Lexer::DebugPrintToken(Token token)
	{
		StringView tokenText{m_File->Content().Data() + token.span.begin, token.span.end - token.span.begin};

		printf("Token: %s, Text: '%.*s'\n", TokenTypeToCStr(token.type), (int)tokenText.Length(), tokenText.Data());
	}

	void Lexer::Advance()
	{
		if (IsEOF())
			return;

		m_CurrentChar++;
	}

	void Lexer::SkipWhitespace()
	{
		while (true)
		{
			switch (GetCurrentChar())
			{
			case ' ':
			case '\r':
			case '\t':
			case '\n':
				Advance();
				break;
			case '/':
				if (GetNextChar() == '/')
				{
					// Skip the rest of the line
					while (!IsAtNewline() && !IsEOF()) Advance();
					break;
				}
				return;
			case '<':
				if (GetNextChar() == '*')
				{
					m_LexingStartOffset = (u32)(m_CurrentChar - m_File->Content().Data());

					Advance(); // consume '<'
					Advance(); // consume '*'

					i32 depth = 1;
					while (depth > 0)
					{
						if (IsEOF())
						{
							Token tok = CreateNewToken(TOKEN_TYPE_INVALID);
							m_Diagnostics->ReportError(tok.span, m_File,
							                           "Unterminated multi-line comment, expected '*>' before the end of the file.");
							return;
						}

						if (GetCurrentChar() == '<' && GetNextChar() == '*')
						{
							Advance(); // consume '<'
							Advance(); // consume '*'

							depth++;

							continue;
						}

						if (GetCurrentChar() == '*' && GetNextChar() == '>')
						{
							Advance(); // consume '*'
							Advance(); // consume '>'

							depth--;

							continue;
						}

						Advance();
					}

					continue;
				}
				return;
			default:
				return;
			};
		}
	}

	Token Lexer::CreateNewToken(TokenType type)
	{
		Span span = {.begin = m_LexingStartOffset, .end = (u32)(m_CurrentChar - m_File->Content().Data())};

		return {.type = type, .span = span};
	}

	Token Lexer::LexIdentifierOrKeyword()
	{
		while (IsCharacterAnAlphanumeric(GetCurrentChar()) || IsCharacterADigit(GetCurrentChar()))
		{
			Advance();
		}

		u32 length = (u32)(m_CurrentChar - m_File->Content().Data()) - m_LexingStartOffset;
		StringView ident{m_File->Content().Data() + m_LexingStartOffset, length};

		ASSERT(length > 0);

		switch (ident.Data()[0])
		{
		case 'b':
			if (ident == "bool")
				return CreateNewToken(TOKEN_TYPE_BOOL_KEYWORD);
			break;

		case 'c':
			if (ident == "cast")
				return CreateNewToken(TOKEN_TYPE_CAST_KEYWORD);
			if (ident == "char")
				return CreateNewToken(TOKEN_TYPE_CHAR_KEYWORD);
			if (ident == "cstring")
				return CreateNewToken(TOKEN_TYPE_CSTRING_KEYWORD);
			break;

		case 'd':
			if (ident == "double")
				return CreateNewToken(TOKEN_TYPE_DOUBLE_KEYWORD);
			break;

		case 'f':
			if (ident == "fn")
				return CreateNewToken(TOKEN_TYPE_FN_KEYWORD);
			if (ident == "float")
				return CreateNewToken(TOKEN_TYPE_FLOAT_KEYWORD);
			if (ident == "false")
				return CreateNewToken(TOKEN_TYPE_FALSE);
			break;

		case 'i':
			if (ident == "int")
				return CreateNewToken(TOKEN_TYPE_INT_KEYWORD);
			if (ident == "intptr")
				return CreateNewToken(TOKEN_TYPE_INTPTR_KEYWORD);
			break;

		case 'l':
			if (ident == "long")
				return CreateNewToken(TOKEN_TYPE_LONG_KEYWORD);
			break;

		case 'r':
			if (ident == "return")
				return CreateNewToken(TOKEN_TYPE_RETURN_KEYWORD);
			if (ident == "rawptr")
				return CreateNewToken(TOKEN_TYPE_RAWPTR_KEYWORD);
			break;

		case 's':
			if (ident == "short")
				return CreateNewToken(TOKEN_TYPE_SHORT_KEYWORD);
			if (ident == "sz")
				return CreateNewToken(TOKEN_TYPE_SZ_KEYWORD);
			if (ident == "string")
				return CreateNewToken(TOKEN_TYPE_STRING_KEYWORD);
			break;

		case 't':
			if (ident == "true")
				return CreateNewToken(TOKEN_TYPE_TRUE);
			break;

		case 'u':
			if (ident == "uchar")
				return CreateNewToken(TOKEN_TYPE_UCHAR_KEYWORD);
			if (ident == "ushort")
				return CreateNewToken(TOKEN_TYPE_USHORT_KEYWORD);
			if (ident == "uint")
				return CreateNewToken(TOKEN_TYPE_UINT_KEYWORD);
			if (ident == "ulong")
				return CreateNewToken(TOKEN_TYPE_ULONG_KEYWORD);
			if (ident == "usz")
				return CreateNewToken(TOKEN_TYPE_USZ_KEYWORD);
			if (ident == "uintptr")
				return CreateNewToken(TOKEN_TYPE_UINTPTR_KEYWORD);
			break;

		case 'v':
			if (ident == "void")
				return CreateNewToken(TOKEN_TYPE_VOID_KEYWORD);
			break;

		case 'p':
			if (ident == "package")
				return CreateNewToken(TOKEN_TYPE_PACKAGE_KEYWORD);
			break;
		}

		return CreateNewToken(TOKEN_TYPE_IDENTIFIER);
	}

	Token Lexer::LexDigit(char firstChar)
	{
		// 12
		while (IsCharacterADigit(GetCurrentChar()))
		{
			Advance();
		}

		u32 integerPartLength = (u32)(m_CurrentChar - m_File->Content().Data()) - m_LexingStartOffset;
		bool hasLeadingZero   = (firstChar == '0' && integerPartLength > 1);

		TokenType type = TOKEN_TYPE_INTEGER;

		// 12.0f 12.0d
		// 12.f  12.d
		// .12f  .12d
		if (GetCurrentChar() == '.')
		{
			Advance(); // consume '.'

			while (IsCharacterADigit(GetCurrentChar()))
			{
				Advance();
			}

			if (GetCurrentChar() == 'f')
			{
				type = TOKEN_TYPE_FLOAT;
				Advance(); // consume 'f'
			}
			else if (GetCurrentChar() == 'd')
			{
				type = TOKEN_TYPE_DOUBLE;
				Advance(); // consume 'd'
			}
			else
			{
				Token error = CreateNewToken(TOKEN_TYPE_INVALID);
				m_Diagnostics->ReportError(error.span, m_File,
				                           "Invalid floating-point literal, expected 'f' or 'd' suffix after the fractional part.");
				return error;
			}
		}

		if (hasLeadingZero)
		{
			Token error = CreateNewToken(TOKEN_TYPE_INVALID);
			m_Diagnostics->ReportError(error.span, m_File, "Invalid numeric literal, leading zeros are not allowed.");
			return error;
		}

		return CreateNewToken(type);
	}

	Token Lexer::LexToken()
	{
		SkipWhitespace();

		m_LexingStartOffset = (u32)(m_CurrentChar - m_File->Content().Data());

		if (IsEOF())
			return CreateNewToken(TOKEN_TYPE_EOF);

		Token token = s_InvalidToken;

		const char c = GetCurrentChar();
		Advance();

		switch (c)
		{
		case '(':
			token = CreateNewToken(TOKEN_TYPE_OPEN_PAREN);
			break;

		case ')':
			token = CreateNewToken(TOKEN_TYPE_CLOSE_PAREN);
			break;

		case '{':
			token = CreateNewToken(TOKEN_TYPE_OPEN_BRACE);
			break;

		case '}':
			token = CreateNewToken(TOKEN_TYPE_CLOSE_BRACE);
			break;

		case '#':
			if (GetCurrentChar() == 'e')
			{
				// Check for '#entrypoint'
				const char* directiveStart = m_CurrentChar;
				while (IsCharacterAnAlphanumeric(GetCurrentChar()))
				{
					Advance();
				}
				u32 length = (u32)(m_CurrentChar - directiveStart);
				StringView directive{directiveStart, length};
				if (directive == "entrypoint")
				{
					token = CreateNewToken(TOKEN_TYPE_ENTRYPOINT_DIRECTIVE);
				}
				else
				{
					Token tok = CreateNewToken(TOKEN_TYPE_INVALID);
					m_Diagnostics->ReportError(tok.span, m_File, "Unknown directive '#{}', did you mean '#entrypoint'?",
					                           std::string_view{directiveStart, length});
					return tok;
				}
			}
			else
			{
				Token tok = CreateNewToken(TOKEN_TYPE_INVALID);
				m_Diagnostics->ReportError(tok.span, m_File, "Unexpected character '#', did you mean '#entrypoint'?");
				return tok;
			}
			break;

		case ';':
			token = CreateNewToken(TOKEN_TYPE_SEMICOLON);
			break;

		case '=':
			token = CreateNewToken(TOKEN_TYPE_EQUALS);
			break;

		case '.':
			if (IsCharacterADigit(GetCurrentChar()))
			{
				// Handle floating-point literals that start with a dot, like .14f or .14d
				m_CurrentChar--;
				token = LexDigit('.');
			}
			else
			{
				token = CreateNewToken(TOKEN_TYPE_DOT);
			}
			break;

		case '!':
			if (GetCurrentChar() == '!')
			{
				Advance();
				token = CreateNewToken(TOKEN_TYPE_BANG_BANG);
			}
			else
			{
				// Standalone '!' not supported yet
				Token tok = CreateNewToken(TOKEN_TYPE_INVALID);
				m_Diagnostics->ReportError(tok.span, m_File, "Unexpected character '!', did you mean '!!'?");
				return tok;
			}
			break;

		default:
			if (IsCharacterADigit(c))
				return LexDigit(c);
			if (IsCharacterAnAlphanumeric(c))
				return LexIdentifierOrKeyword();
			break;
		};

		if (token.type == TOKEN_TYPE_INVALID)
		{
			Token tok = CreateNewToken(TOKEN_TYPE_INVALID);
			m_Diagnostics->ReportError(tok.span, m_File, "Unexpected character '{}', this character is not recognized as valid in the language.", c);
			return tok;
		}

		return token;
	}

} // namespace Wandelt
