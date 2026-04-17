#include "Token.hpp"

namespace Wandelt
{

	const char* TokenTypeToCStr(TokenType type)
	{
		switch (type)
		{
		case TOKEN_TYPE_INVALID:
			ASSERT(false, "Invalid token type!");
			break;

		case TOKEN_TYPE_PACKAGE_KEYWORD:
			return "package";

		case TOKEN_TYPE_RETURN_KEYWORD:
			return "return";

		case TOKEN_TYPE_AS_KEYWORD:
			return "as";

		case TOKEN_TYPE_ENTRYPOINT_DIRECTIVE:
			return "#entrypoint";

		case TOKEN_TYPE_VOID_KEYWORD:
			return "void";

		case TOKEN_TYPE_BOOL_KEYWORD:
			return "bool";

		case TOKEN_TYPE_CHAR_KEYWORD:
			return "char";

		case TOKEN_TYPE_UCHAR_KEYWORD:
			return "uchar";

		case TOKEN_TYPE_SHORT_KEYWORD:
			return "short";

		case TOKEN_TYPE_USHORT_KEYWORD:
			return "ushort";

		case TOKEN_TYPE_INT_KEYWORD:
			return "int";

		case TOKEN_TYPE_UINT_KEYWORD:
			return "uint";

		case TOKEN_TYPE_LONG_KEYWORD:
			return "long";

		case TOKEN_TYPE_ULONG_KEYWORD:
			return "ulong";

		case TOKEN_TYPE_SZ_KEYWORD:
			return "sz";

		case TOKEN_TYPE_USZ_KEYWORD:
			return "usz";

		case TOKEN_TYPE_INTPTR_KEYWORD:
			return "intptr";

		case TOKEN_TYPE_UINTPTR_KEYWORD:
			return "uintptr";

		case TOKEN_TYPE_FLOAT_KEYWORD:
			return "float";

		case TOKEN_TYPE_DOUBLE_KEYWORD:
			return "double";

		case TOKEN_TYPE_STRING_KEYWORD:
			return "string";

		case TOKEN_TYPE_CSTRING_KEYWORD:
			return "cstring";

		case TOKEN_TYPE_RAWPTR_KEYWORD:
			return "rawptr";

		case TOKEN_TYPE_OPEN_PAREN:
			return "(";

		case TOKEN_TYPE_CLOSE_PAREN:
			return ")";

		case TOKEN_TYPE_OPEN_BRACE:
			return "{";

		case TOKEN_TYPE_CLOSE_BRACE:
			return "}";

		case TOKEN_TYPE_SEMICOLON:
			return ";";

		case TOKEN_TYPE_EQUALS:
			return "=";

		case TOKEN_TYPE_DOT:
			return ".";

		case TOKEN_TYPE_BANG_BANG:
			return "!!";

		case TOKEN_TYPE_IDENTIFIER:
			return "<identifier>";

		case TOKEN_TYPE_INTEGER:
			return "<integer>";

		case TOKEN_TYPE_FLOAT:
			return "<float>";

		case TOKEN_TYPE_DOUBLE:
			return "<double>";

		case TOKEN_TYPE_TRUE:
			return "true";

		case TOKEN_TYPE_FALSE:
			return "false";

		case TOKEN_TYPE_EOF:
			return "<eof>";

		case TOKEN_TYPE_COUNT:
			ASSERT(false, "Invalid token type!");

		default:
			break;
		}

		UNREACHABLE();
	}

	Span Span::Extend(Span a, Span b)
	{
		return {a.begin, b.end};
	}

} // namespace Wandelt
