#pragma once

#include "Wandelt/Defines.hpp"

namespace Wandelt
{

	enum TokenType : u32
	{
		TOKEN_TYPE_INVALID = 0,

		// Keywords
		TOKEN_TYPE_PACKAGE_KEYWORD, // 'package'
		TOKEN_TYPE_RETURN_KEYWORD,  // 'return'
		TOKEN_TYPE_AS_KEYWORD,      // 'as'

		// Directives
		TOKEN_TYPE_ENTRYPOINT_DIRECTIVE, // '#entrypoint'

		// Built-in types
		TOKEN_TYPE_VOID_KEYWORD,    // 'void'
		TOKEN_TYPE_BOOL_KEYWORD,    // 'bool'
		TOKEN_TYPE_CHAR_KEYWORD,    // 'char'
		TOKEN_TYPE_UCHAR_KEYWORD,   // 'uchar'
		TOKEN_TYPE_SHORT_KEYWORD,   // 'short'
		TOKEN_TYPE_USHORT_KEYWORD,  // 'ushort'
		TOKEN_TYPE_INT_KEYWORD,     // 'int'
		TOKEN_TYPE_UINT_KEYWORD,    // 'uint'
		TOKEN_TYPE_LONG_KEYWORD,    // 'long'
		TOKEN_TYPE_ULONG_KEYWORD,   // 'ulong'
		TOKEN_TYPE_SZ_KEYWORD,      // 'sz'
		TOKEN_TYPE_USZ_KEYWORD,     // 'usz'
		TOKEN_TYPE_INTPTR_KEYWORD,  // 'intptr'
		TOKEN_TYPE_UINTPTR_KEYWORD, // 'uintptr'
		TOKEN_TYPE_FLOAT_KEYWORD,   // 'float'
		TOKEN_TYPE_DOUBLE_KEYWORD,  // 'double'
		TOKEN_TYPE_STRING_KEYWORD,  // 'string'
		TOKEN_TYPE_CSTRING_KEYWORD, // 'cstring'
		TOKEN_TYPE_RAWPTR_KEYWORD,  // 'rawptr'

		// Single-character tokens
		TOKEN_TYPE_OPEN_PAREN,  //  (
		TOKEN_TYPE_CLOSE_PAREN, //  )
		TOKEN_TYPE_OPEN_BRACE,  //  {
		TOKEN_TYPE_CLOSE_BRACE, //  }
		TOKEN_TYPE_SEMICOLON,   //  ;
		TOKEN_TYPE_EQUALS,      //  =
		TOKEN_TYPE_DOT,         //  .

		// Double-character tokens
		TOKEN_TYPE_BANG_BANG, //  !!

		// Literals
		TOKEN_TYPE_IDENTIFIER, // ident
		TOKEN_TYPE_INTEGER,    // 123
		TOKEN_TYPE_FLOAT,      // 3.14f, 3.f, .14f
		TOKEN_TYPE_DOUBLE,     // 3.14d, 3.d, .14d
		TOKEN_TYPE_TRUE,       // 'true'
		TOKEN_TYPE_FALSE,      // 'false'

		// Other
		TOKEN_TYPE_EOF,

		TOKEN_TYPE_COUNT
	};

	const char* TokenTypeToCStr(TokenType type);

	struct Span
	{
		u32 begin;
		u32 end;

		static Span Extend(Span a, Span b);
	};

	struct Token
	{
		TokenType type;
		Span span;
	};

} // namespace Wandelt
