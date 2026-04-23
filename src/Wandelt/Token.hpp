#pragma once

#include "Wandelt/Defines.hpp"

namespace Wandelt
{

	enum TokenType : u32
	{
		TOKEN_TYPE_INVALID = 0,

		// Keywords
		TOKEN_TYPE_PACKAGE_KEYWORD,  // 'package'
		TOKEN_TYPE_RETURN_KEYWORD,   // 'return'
		TOKEN_TYPE_CAST_KEYWORD,     // 'cast'
		TOKEN_TYPE_FN_KEYWORD,       // 'fn'
		TOKEN_TYPE_DISCARD_KEYWORD,  // 'discard'
		TOKEN_TYPE_IF_KEYWORD,       // 'if'
		TOKEN_TYPE_ELSE_KEYWORD,     // 'else'
		TOKEN_TYPE_WHILE_KEYWORD,    // 'while'
		TOKEN_TYPE_FOR_KEYWORD,      // 'for'
		TOKEN_TYPE_BREAK_KEYWORD,    // 'break'
		TOKEN_TYPE_CONTINUE_KEYWORD, // 'continue'

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
		TOKEN_TYPE_OPEN_PAREN,   //  (
		TOKEN_TYPE_CLOSE_PAREN,  //  )
		TOKEN_TYPE_OPEN_BRACE,   //  {
		TOKEN_TYPE_CLOSE_BRACE,  //  }
		TOKEN_TYPE_SEMICOLON,    //  ;
		TOKEN_TYPE_COMMA,        //  ,
		TOKEN_TYPE_DOT,          //  .
		TOKEN_TYPE_EQUALS,       //  =
		TOKEN_TYPE_PLUS,         //  +
		TOKEN_TYPE_MINUS,        //  -
		TOKEN_TYPE_STAR,         //  *
		TOKEN_TYPE_SLASH,        //  /
		TOKEN_TYPE_GREATER,      //  >
		TOKEN_TYPE_LESS,         //  <
		TOKEN_TYPE_SINGLE_QUOTE, //  '
		TOKEN_TYPE_DOUBLE_QUOTE, //  "

		// Double-character tokens
		TOKEN_TYPE_BANG_BANG,     //  !!
		TOKEN_TYPE_GREATER_EQUAL, //  >=
		TOKEN_TYPE_LESS_EQUAL,    //  <=
		TOKEN_TYPE_EQUAL_EQUAL,   //  ==
		TOKEN_TYPE_BANG_EQUAL,    //  !=
		TOKEN_TYPE_PLUS_EQUAL,    //  +=
		TOKEN_TYPE_MINUS_EQUAL,   //  -=
		TOKEN_TYPE_STAR_EQUAL,    //  *=
		TOKEN_TYPE_SLASH_EQUAL,   //  /=
		TOKEN_TYPE_PLUS_PLUS,     //  ++
		TOKEN_TYPE_MINUS_MINUS,   //  --

		// Literals
		TOKEN_TYPE_IDENTIFIER, // ident
		TOKEN_TYPE_INTEGER,    // 123
		TOKEN_TYPE_FLOAT,      // 3.14f, 3.f, .14f
		TOKEN_TYPE_DOUBLE,     // 3.14d, 3.d, .14d
		TOKEN_TYPE_TRUE,       // 'true'
		TOKEN_TYPE_FALSE,      // 'false'
		TOKEN_TYPE_CHARACTER,  // 'a', '\n', etc.
		TOKEN_TYPE_STRING,     // "hello world"

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
