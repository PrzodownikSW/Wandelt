#include "LexerTests.hpp"

#include "TestingFramework.hpp"

namespace Wandelt
{

	static constexpr i32 MaxTokens = 512;

	struct TokenList
	{
		Token items[MaxTokens];
		i32 count      = 0;
		bool truncated = false;
	};

	struct SingleTokenCase
	{
		const char* source;
		TokenType type;
		const char* lexeme;
	};

	struct ExpectedToken
	{
		TokenType type;
		const char* lexeme;
	};

	static TokenList LexSource(Allocator* alloc, const char* source, Diagnostics* diagnostics)
	{
		File file = MakeTestFile(alloc, source);
		Lexer lexer{&file, diagnostics};
		TokenList tokens;

		while (tokens.count < MaxTokens)
		{
			Token tok                    = lexer.PeekToken();
			tokens.items[tokens.count++] = tok;
			if (tok.type == TOKEN_TYPE_EOF)
				break;

			lexer.EatToken();
		}

		if (tokens.count == MaxTokens && tokens.items[MaxTokens - 1].type != TOKEN_TYPE_EOF)
			tokens.truncated = true;

		return tokens;
	}

	static StringView TokenLexeme(const char* source, Token tok)
	{
		u32 tokenLength = tok.span.end - tok.span.begin;

		return StringView{source + tok.span.begin, tokenLength};
	}

	TEST(EmptyAndCommentOnlyInputs)
	{
		(void)alloc;
		Diagnostics diag;

		const char* cases[] = {
		    "",
		    "   \t\n\r  \n  ",
		    "// This is a comment\n// Another comment\n",
		    "<* This is a\nmultiline comment *>",
		    "<* This is a\nmultiline comment with a nested comment <* nested *>\nEnd of outer comment *>",
		};

		for (u32 index = 0; index < ArraySize(cases); index++)
		{
			TokenList tokens = LexSource(alloc, cases[index], &diag);

			ASSERT_FALSE(tokens.truncated);
			ASSERT_EQ(tokens.count, 1);
			ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_EOF);
		}

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(SingleCharacterTokens)
	{
		Diagnostics diag;

		const SingleTokenCase cases[] = {
		    {"(", TOKEN_TYPE_OPEN_PAREN, "("},  {")", TOKEN_TYPE_CLOSE_PAREN, ")"}, {"{", TOKEN_TYPE_OPEN_BRACE, "{"},
		    {"}", TOKEN_TYPE_CLOSE_BRACE, "}"}, {";", TOKEN_TYPE_SEMICOLON, ";"},   {"=", TOKEN_TYPE_EQUALS, "="},
		    {".", TOKEN_TYPE_DOT, "."},
		};

		for (u32 index = 0; index < ArraySize(cases); index++)
		{
			const SingleTokenCase& testCase = cases[index];
			TokenList tokens                = LexSource(alloc, testCase.source, &diag);

			ASSERT_FALSE(tokens.truncated);
			ASSERT_EQ(tokens.count, 2);
			ASSERT_EQ(tokens.items[0].type, testCase.type);
			ASSERT_STR_EQ(TokenLexeme(testCase.source, tokens.items[0]), testCase.lexeme);
			ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_EOF);
		}

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(Identifiers)
	{
		Diagnostics diag;

		const SingleTokenCase cases[] = {
		    {"main", TOKEN_TYPE_IDENTIFIER, "main"}, {"_my_var", TOKEN_TYPE_IDENTIFIER, "_my_var"},
		    {"x123", TOKEN_TYPE_IDENTIFIER, "x123"}, {"x", TOKEN_TYPE_IDENTIFIER, "x"},
		    {"_", TOKEN_TYPE_IDENTIFIER, "_"},       {"TOKEN_TYPE_EOF", TOKEN_TYPE_IDENTIFIER, "TOKEN_TYPE_EOF"},
		};

		for (u32 index = 0; index < ArraySize(cases); index++)
		{
			const SingleTokenCase& testCase = cases[index];
			TokenList tokens                = LexSource(alloc, testCase.source, &diag);

			ASSERT_FALSE(tokens.truncated);
			ASSERT_EQ(tokens.count, 2);
			ASSERT_EQ(tokens.items[0].type, testCase.type);
			ASSERT_STR_EQ(TokenLexeme(testCase.source, tokens.items[0]), testCase.lexeme);
			ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_EOF);
		}

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(KeywordAndBuiltinTypeTokens)
	{
		Diagnostics diag;

		const SingleTokenCase cases[] = {
		    {"package", TOKEN_TYPE_PACKAGE_KEYWORD, "package"},
		    {"return", TOKEN_TYPE_RETURN_KEYWORD, "return"},
		    {"cast", TOKEN_TYPE_CAST_KEYWORD, "cast"},
		    {"fn", TOKEN_TYPE_FN_KEYWORD, "fn"},
		    {"discard", TOKEN_TYPE_DISCARD_KEYWORD, "discard"},
		    {"void", TOKEN_TYPE_VOID_KEYWORD, "void"},
		    {"bool", TOKEN_TYPE_BOOL_KEYWORD, "bool"},
		    {"char", TOKEN_TYPE_CHAR_KEYWORD, "char"},
		    {"uchar", TOKEN_TYPE_UCHAR_KEYWORD, "uchar"},
		    {"short", TOKEN_TYPE_SHORT_KEYWORD, "short"},
		    {"ushort", TOKEN_TYPE_USHORT_KEYWORD, "ushort"},
		    {"int", TOKEN_TYPE_INT_KEYWORD, "int"},
		    {"uint", TOKEN_TYPE_UINT_KEYWORD, "uint"},
		    {"long", TOKEN_TYPE_LONG_KEYWORD, "long"},
		    {"ulong", TOKEN_TYPE_ULONG_KEYWORD, "ulong"},
		    {"sz", TOKEN_TYPE_SZ_KEYWORD, "sz"},
		    {"usz", TOKEN_TYPE_USZ_KEYWORD, "usz"},
		    {"intptr", TOKEN_TYPE_INTPTR_KEYWORD, "intptr"},
		    {"uintptr", TOKEN_TYPE_UINTPTR_KEYWORD, "uintptr"},
		    {"float", TOKEN_TYPE_FLOAT_KEYWORD, "float"},
		    {"double", TOKEN_TYPE_DOUBLE_KEYWORD, "double"},
		    {"string", TOKEN_TYPE_STRING_KEYWORD, "string"},
		    {"cstring", TOKEN_TYPE_CSTRING_KEYWORD, "cstring"},
		    {"rawptr", TOKEN_TYPE_RAWPTR_KEYWORD, "rawptr"},
		    {"true", TOKEN_TYPE_TRUE, "true"},
		    {"false", TOKEN_TYPE_FALSE, "false"},
		};

		for (u32 index = 0; index < ArraySize(cases); index++)
		{
			const SingleTokenCase& testCase = cases[index];
			TokenList tokens                = LexSource(alloc, testCase.source, &diag);

			ASSERT_FALSE(tokens.truncated);
			ASSERT_EQ(tokens.count, 2);
			ASSERT_EQ(tokens.items[0].type, testCase.type);
			ASSERT_STR_EQ(TokenLexeme(testCase.source, tokens.items[0]), testCase.lexeme);
			ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_EOF);
		}

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(DirectiveToken)
	{
		Diagnostics diag;
		const char* source = "#entrypoint";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 2);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_ENTRYPOINT_DIRECTIVE);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "#entrypoint");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(KeywordPrefixesRemainIdentifiers)
	{
		Diagnostics diag;

		const char* cases[] = {
		    "packageName", "returnValue", "castValue", "boolish", "trueValue", "false_alarm", "uintptr2", "rawptrValue", "discarded",
		};

		for (u32 index = 0; index < ArraySize(cases); index++)
		{
			TokenList tokens = LexSource(alloc, cases[index], &diag);

			ASSERT_FALSE(tokens.truncated);
			ASSERT_EQ(tokens.count, 2);
			ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
			ASSERT_STR_EQ(TokenLexeme(cases[index], tokens.items[0]), cases[index]);
			ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_EOF);
		}

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(IntegerLiterals)
	{
		Diagnostics diag;

		const SingleTokenCase cases[] = {
		    {"0", TOKEN_TYPE_INTEGER, "0"},
		    {"42", TOKEN_TYPE_INTEGER, "42"},
		    {"1234567890", TOKEN_TYPE_INTEGER, "1234567890"},
		};

		for (u32 index = 0; index < ArraySize(cases); index++)
		{
			const SingleTokenCase& testCase = cases[index];
			TokenList tokens                = LexSource(alloc, testCase.source, &diag);

			ASSERT_FALSE(tokens.truncated);
			ASSERT_EQ(tokens.count, 2);
			ASSERT_EQ(tokens.items[0].type, testCase.type);
			ASSERT_STR_EQ(TokenLexeme(testCase.source, tokens.items[0]), testCase.lexeme);
			ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_EOF);
		}

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(FloatingPointLiterals)
	{
		Diagnostics diag;

		const SingleTokenCase cases[] = {
		    {"3.14f", TOKEN_TYPE_FLOAT, "3.14f"},        {"3.f", TOKEN_TYPE_FLOAT, "3.f"},  {".14f", TOKEN_TYPE_FLOAT, ".14f"},
		    {"2.71828d", TOKEN_TYPE_DOUBLE, "2.71828d"}, {"2.d", TOKEN_TYPE_DOUBLE, "2.d"}, {".71828d", TOKEN_TYPE_DOUBLE, ".71828d"},
		};

		for (u32 index = 0; index < ArraySize(cases); index++)
		{
			const SingleTokenCase& testCase = cases[index];
			TokenList tokens                = LexSource(alloc, testCase.source, &diag);

			ASSERT_FALSE(tokens.truncated);
			ASSERT_EQ(tokens.count, 2);
			ASSERT_EQ(tokens.items[0].type, testCase.type);
			ASSERT_STR_EQ(TokenLexeme(testCase.source, tokens.items[0]), testCase.lexeme);
			ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_EOF);
		}

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(DoubleCharacterTokens)
	{
		Diagnostics diag;

		const SingleTokenCase cases[] = {
		    {"!!", TOKEN_TYPE_BANG_BANG, "!!"},
		};

		for (u32 index = 0; index < ArraySize(cases); index++)
		{
			const SingleTokenCase& testCase = cases[index];
			TokenList tokens                = LexSource(alloc, testCase.source, &diag);

			ASSERT_FALSE(tokens.truncated);
			ASSERT_EQ(tokens.count, 2);
			ASSERT_EQ(tokens.items[0].type, testCase.type);
			ASSERT_STR_EQ(TokenLexeme(testCase.source, tokens.items[0]), testCase.lexeme);
			ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_EOF);
		}

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(LeadingZerosOnNumericLiteralsAreRejected)
	{
		const char* sources[] = {
		    "007", "000", "0123", "00.5f", "00000.2f", "007.14d",
		};

		for (u32 index = 0; index < ArraySize(sources); index++)
		{
			Diagnostics diag;
			Diagnostics::CaptureScope capture(diag);
			TokenList tokens = LexSource(alloc, sources[index], &diag);

			ASSERT_FALSE(tokens.truncated);
			ASSERT_EQ(tokens.items[tokens.count - 1].type, TOKEN_TYPE_EOF);
			ASSERT_EQ(diag.CapturedCount(), 1u);

			Diagnostics::Entry* entry = diag.GetCaptured(0);
			ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
			ASSERT_STR_CONTAINS(entry->message, "leading zeros are not allowed");
		}
	}

	TEST(LeadingZeroDiagnosticsUseDisplayColumnsForTabs)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "\t012";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.items[tokens.count - 1].type, TOKEN_TYPE_EOF);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_EQ(entry->line, 1u);
		ASSERT_EQ(entry->col, 5u);
		ASSERT_STR_CONTAINS(entry->message, "leading zeros are not allowed");
	}

	TEST(DiagnosticRendererShowsSpanAtColumnOne)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "012";
		File file          = MakeTestFile(alloc, source);
		Lexer lexer{&file, &diag};

		Token tok = lexer.PeekToken();
		ASSERT_EQ(tok.type, TOKEN_TYPE_INVALID);

		std::string rendered = diag.FormatAtLocation(tok.span, &file, "Invalid numeric literal, leading zeros are not allowed.",
		                                             Diagnostics::Severity::Error, 80, false);

		ASSERT_STR_CONTAINS(rendered.c_str(), "test.wdt:1:1: error: Invalid numeric literal, leading zeros are not allowed.\n");
		ASSERT_STR_CONTAINS(rendered.c_str(), " 1 | 012\n");
		ASSERT_STR_CONTAINS(rendered.c_str(), "   | ^~~\n");
	}

	TEST(DiagnosticRendererExpandsTabsBeforeCaret)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "\t012";
		File file          = MakeTestFile(alloc, source);
		Lexer lexer{&file, &diag};

		Token tok = lexer.PeekToken();
		ASSERT_EQ(tok.type, TOKEN_TYPE_INVALID);

		std::string rendered = diag.FormatAtLocation(tok.span, &file, "Invalid numeric literal, leading zeros are not allowed.",
		                                             Diagnostics::Severity::Error, 80, false);

		ASSERT_STR_CONTAINS(rendered.c_str(), "test.wdt:1:5: error: Invalid numeric literal, leading zeros are not allowed.\n");
		ASSERT_STR_CONTAINS(rendered.c_str(), " 1 |     012\n");
		ASSERT_STR_CONTAINS(rendered.c_str(), "   |     ^~~\n");
	}

	TEST(DiagnosticRendererClipsLongLinesOnTheRight)
	{
		Diagnostics diag;
		const char* source = "ab012cdefghijklmnopqrstu";
		File file          = MakeTestFile(alloc, source);
		Span span{.begin = 2, .end = 5};

		std::string rendered =
		    diag.FormatAtLocation(span, &file, "Invalid numeric literal, leading zeros are not allowed.", Diagnostics::Severity::Error, 24, false);
		std::string expectedCaret = "   | ";
		expectedCaret.append(2, ' ');
		expectedCaret += "^~~\n";

		ASSERT_STR_CONTAINS(rendered.c_str(), " 1 | ab012cdefghijklmn...\n");
		ASSERT_STR_CONTAINS(rendered.c_str(), expectedCaret.c_str());
	}

	TEST(DiagnosticRendererClipsLongLinesOnBothSides)
	{
		Diagnostics diag;
		const char* source = "abcdefghijkl012mnopqrstuvwxyz";
		File file          = MakeTestFile(alloc, source);
		Span span{.begin = 12, .end = 15};

		std::string rendered =
		    diag.FormatAtLocation(span, &file, "Invalid numeric literal, leading zeros are not allowed.", Diagnostics::Severity::Error, 24, false);
		std::string expectedCaret = "   | ";
		expectedCaret.append(8, ' ');
		expectedCaret += "^~~\n";

		ASSERT_STR_CONTAINS(rendered.c_str(), " 1 | ...hijkl012mnopqr...\n");
		ASSERT_STR_CONTAINS(rendered.c_str(), expectedCaret.c_str());
	}

	TEST(ZeroFloatLiterals)
	{
		Diagnostics diag;

		const SingleTokenCase cases[] = {
		    {"0.0f", TOKEN_TYPE_FLOAT, "0.0f"},
		    {"0.0d", TOKEN_TYPE_DOUBLE, "0.0d"},
		    {"0.f", TOKEN_TYPE_FLOAT, "0.f"},
		    {".0f", TOKEN_TYPE_FLOAT, ".0f"},
		};

		for (u32 index = 0; index < ArraySize(cases); index++)
		{
			const SingleTokenCase& testCase = cases[index];
			TokenList tokens                = LexSource(alloc, testCase.source, &diag);

			ASSERT_FALSE(tokens.truncated);
			ASSERT_EQ(tokens.count, 2);
			ASSERT_EQ(tokens.items[0].type, testCase.type);
			ASSERT_STR_EQ(TokenLexeme(testCase.source, tokens.items[0]), testCase.lexeme);
			ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_EOF);
		}

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(MixedTokenSequence)
	{
		Diagnostics diag;
		const char* source             = "package demo\n#entrypoint\nreturn true";
		const ExpectedToken expected[] = {
		    {TOKEN_TYPE_PACKAGE_KEYWORD, "package"}, {TOKEN_TYPE_IDENTIFIER, "demo"}, {TOKEN_TYPE_ENTRYPOINT_DIRECTIVE, "#entrypoint"},
		    {TOKEN_TYPE_RETURN_KEYWORD, "return"},   {TOKEN_TYPE_TRUE, "true"},
		};
		TokenList tokens = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, (i32)ArraySize(expected) + 1);

		for (u32 index = 0; index < ArraySize(expected); index++)
		{
			ASSERT_EQ(tokens.items[index].type, expected[index].type);
			ASSERT_STR_EQ(TokenLexeme(source, tokens.items[index]), expected[index].lexeme);
		}

		ASSERT_EQ(tokens.items[ArraySize(expected)].type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(LineCommentBetweenTokens)
	{
		Diagnostics diag;
		const char* source = "foo // ignore this\nbar";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 3);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "foo");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "bar");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(MultiLineCommentBetweenTokens)
	{
		Diagnostics diag;
		const char* source = "foo <* skip this *> bar";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 3);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "foo");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "bar");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(NestedMultiLineCommentBetweenTokens)
	{
		Diagnostics diag;
		const char* source = "foo <* outer <* nested *> still outer *> bar";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 3);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "foo");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "bar");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(CarriageReturnNewlineSeparatesTokens)
	{
		Diagnostics diag;
		const char* source = "a\r\nb";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 3);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "a");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "b");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(TokensSeparatedByWhitespace)
	{
		Diagnostics diag;
		const char* source = "  foo   123   ;  ";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 4);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "foo");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_INTEGER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "123");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_SEMICOLON);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[2]), ";");
		ASSERT_EQ(tokens.items[3].type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(DotFollowedByNonDigit)
	{
		Diagnostics diag;
		const char* source = "foo.bar";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 4);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "foo");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_DOT);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), ".");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[2]), "bar");
		ASSERT_EQ(tokens.items[3].type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(EmptyMultiLineComment)
	{
		Diagnostics diag;
		const char* source = "<**>";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 1);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(LineCommentAtEofWithoutNewline)
	{
		Diagnostics diag;
		const char* source = "foo // trailing";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 2);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "foo");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(MultiLineCommentEndingExactlyAtEof)
	{
		Diagnostics diag;
		const char* source = "foo <* x *>";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 2);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "foo");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(MultiLineCommentIgnoresLooseSpecialChars)
	{
		Diagnostics diag;
		const char* source = "foo <* a * b < c > d *> bar";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 3);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "foo");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "bar");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(LoneCarriageReturnSeparatesTokens)
	{
		Diagnostics diag;
		const char* source = "a\rb";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 3);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "a");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "b");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(ValidTokenLineAndColumnTracking)
	{
		Diagnostics diag;
		const char* source = "a\n  b\r\n   c";
		File file          = MakeTestFile(alloc, source);
		Lexer lexer{&file, &diag};

		Token a           = lexer.PeekToken();
		FileLocation aLoc = file.ResolveLocation(a.span.begin);
		ASSERT_EQ(a.type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_EQ(aLoc.row, 1u);
		ASSERT_EQ(aLoc.col, 1u);
		lexer.EatToken();

		Token b           = lexer.PeekToken();
		FileLocation bLoc = file.ResolveLocation(b.span.begin);
		ASSERT_EQ(b.type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_EQ(bLoc.row, 2u);
		ASSERT_EQ(bLoc.col, 3u);
		lexer.EatToken();

		Token c           = lexer.PeekToken();
		FileLocation cLoc = file.ResolveLocation(c.span.begin);
		ASSERT_EQ(c.type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_EQ(cLoc.row, 3u);
		ASSERT_EQ(cLoc.col, 4u);

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(PeekReturnsSameToken)
	{
		Diagnostics diag;
		const char* source = "abc";
		File file          = MakeTestFile(alloc, source);
		Lexer lexer{&file, &diag};

		Token firstPeek  = lexer.PeekToken();
		Token secondPeek = lexer.PeekToken();

		ASSERT_EQ(firstPeek.type, secondPeek.type);
		ASSERT_EQ(firstPeek.span.begin, secondPeek.span.begin);
		ASSERT_EQ(firstPeek.span.end, secondPeek.span.end);
		ASSERT_STR_EQ(TokenLexeme(source, firstPeek), "abc");
		ASSERT_STR_EQ(TokenLexeme(source, secondPeek), "abc");
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(EatThenPeekAdvances)
	{
		Diagnostics diag;
		const char* source = "a b";
		File file          = MakeTestFile(alloc, source);
		Lexer lexer{&file, &diag};

		Token first = lexer.PeekToken();
		ASSERT_STR_EQ(TokenLexeme(source, first), "a");

		lexer.EatToken();

		Token second = lexer.PeekToken();
		ASSERT_EQ(second.type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, second), "b");
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(EatAllReachesEof)
	{
		Diagnostics diag;
		const char* source = "x y z";
		File file          = MakeTestFile(alloc, source);
		Lexer lexer{&file, &diag};

		for (int index = 0; index < 3; index++)
		{
			ASSERT_TRUE(lexer.PeekToken().type != TOKEN_TYPE_EOF);
			lexer.EatToken();
		}

		ASSERT_EQ(lexer.PeekToken().type, TOKEN_TYPE_EOF);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(PeekAtOffsetDoesNotRequirePriming)
	{
		Diagnostics diag;
		const char* source = "a b c";
		File file          = MakeTestFile(alloc, source);
		Lexer lexer{&file, &diag};

		ASSERT_STR_EQ(TokenLexeme(source, lexer.PeekTokenAtOffset(0)), "a");
		ASSERT_STR_EQ(TokenLexeme(source, lexer.PeekTokenAtOffset(1)), "b");
		ASSERT_STR_EQ(TokenLexeme(source, lexer.PeekTokenAtOffset(2)), "c");
		ASSERT_EQ(lexer.PeekTokenAtOffset(3).type, TOKEN_TYPE_EOF);

		Token current = lexer.PeekToken();
		ASSERT_STR_EQ(TokenLexeme(source, current), "a");

		lexer.EatToken();
		ASSERT_STR_EQ(TokenLexeme(source, lexer.PeekToken()), "b");
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(PeekAtOffsetFromPrimedLexerDoesNotConsume)
	{
		Diagnostics diag;
		const char* source = "a b c";
		File file          = MakeTestFile(alloc, source);
		Lexer lexer{&file, &diag};

		Token current = lexer.PeekToken();
		ASSERT_STR_EQ(TokenLexeme(source, current), "a");
		ASSERT_STR_EQ(TokenLexeme(source, lexer.PeekTokenAtOffset(1)), "b");
		ASSERT_STR_EQ(TokenLexeme(source, lexer.PeekTokenAtOffset(2)), "c");
		ASSERT_EQ(lexer.PeekTokenAtOffset(3).type, TOKEN_TYPE_EOF);

		ASSERT_STR_EQ(TokenLexeme(source, lexer.PeekToken()), "a");
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(EatTokenBeforePeekIsSafe)
	{
		Diagnostics diag;
		const char* source = "a b";
		File file          = MakeTestFile(alloc, source);
		Lexer lexer{&file, &diag};

		lexer.EatToken();

		Token first = lexer.PeekToken();
		ASSERT_EQ(first.type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, first), "a");

		lexer.EatToken();

		Token second = lexer.PeekToken();
		ASSERT_EQ(second.type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, second), "b");

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(PeekAtOffsetFarPastEof)
	{
		Diagnostics diag;
		const char* source = "a";
		File file          = MakeTestFile(alloc, source);
		Lexer lexer{&file, &diag};

		ASSERT_EQ(lexer.PeekTokenAtOffset(5).type, TOKEN_TYPE_EOF);
		ASSERT_EQ(lexer.PeekTokenAtOffset(10).type, TOKEN_TYPE_EOF);
		ASSERT_EQ(lexer.PeekTokenAtOffset(100).type, TOKEN_TYPE_EOF);

		// The lexer should still be positioned at the first token after repeated peeks past EOF.
		ASSERT_STR_EQ(TokenLexeme(source, lexer.PeekToken()), "a");

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(PeekAtOffsetOnEmptyStream)
	{
		Diagnostics diag;
		const char* source = "";
		File file          = MakeTestFile(alloc, source);
		Lexer lexer{&file, &diag};

		ASSERT_EQ(lexer.PeekTokenAtOffset(0).type, TOKEN_TYPE_EOF);
		ASSERT_EQ(lexer.PeekTokenAtOffset(0).type, TOKEN_TYPE_EOF);
		ASSERT_EQ(lexer.PeekTokenAtOffset(3).type, TOKEN_TYPE_EOF);
		ASSERT_EQ(lexer.PeekTokenAtOffset(99).type, TOKEN_TYPE_EOF);

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(PeekAtOffsetOnConsumedStream)
	{
		Diagnostics diag;
		const char* source = "only";
		File file          = MakeTestFile(alloc, source);
		Lexer lexer{&file, &diag};

		ASSERT_STR_EQ(TokenLexeme(source, lexer.PeekToken()), "only");
		lexer.EatToken();

		ASSERT_EQ(lexer.PeekTokenAtOffset(0).type, TOKEN_TYPE_EOF);
		ASSERT_EQ(lexer.PeekTokenAtOffset(2).type, TOKEN_TYPE_EOF);
		ASSERT_EQ(lexer.PeekTokenAtOffset(0).type, TOKEN_TYPE_EOF);

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(UnterminatedMultiLineComment)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "<* unterminated comment";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 1);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_EOF);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_EQ(entry->line, 1u);
		ASSERT_EQ(entry->col, 1u);
		ASSERT_STR_CONTAINS(entry->message, "Unterminated multi-line comment, expected '*>' before the end of the file.");
	}

	TEST(UnterminatedCommentAfterToken)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "foo <* unterminated comment";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 2);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "foo");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_EOF);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_EQ(entry->line, 1u);
		ASSERT_EQ(entry->col, 5u);
		ASSERT_STR_CONTAINS(entry->message, "Unterminated multi-line comment, expected '*>' before the end of the file.");
	}

	TEST(StandaloneBangReportsDiagnosticAndRecovers)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "! 42";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 3);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_INVALID);
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_INTEGER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "42");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_EOF);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_EQ(entry->line, 1u);
		ASSERT_EQ(entry->col, 1u);
		ASSERT_STR_CONTAINS(entry->message, "Unexpected character '!', did you mean '!!'?");
	}

	TEST(UnrecognizedCharacterReportsDiagnosticAndRecovers)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "@ 42";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 3);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_INVALID);
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_INTEGER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "42");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_EOF);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_EQ(entry->line, 1u);
		ASSERT_EQ(entry->col, 1u);
		ASSERT_STR_CONTAINS(entry->message, "Unexpected character '@', this character is not recognized as valid in the language.");
	}

	TEST(FloatLiteralMissingSuffixReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "3.14";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.items[tokens.count - 1].type, TOKEN_TYPE_EOF);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_EQ(entry->line, 1u);
		ASSERT_EQ(entry->col, 1u);
		ASSERT_STR_CONTAINS(entry->message, "Invalid floating-point literal, expected 'f' or 'd' suffix");
	}

	TEST(UnknownDirectiveReportsDiagnosticAndRecovers)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "#entry 42";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 3);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_INVALID);
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_INTEGER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "42");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_EOF);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_EQ(entry->line, 1u);
		ASSERT_EQ(entry->col, 1u);
		ASSERT_STR_CONTAINS(entry->message, "Unknown directive '#entry'");
	}

	TEST(HashWithoutDirectiveReportsDiagnosticAndRecovers)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "# 42";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 3);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_INVALID);
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_INTEGER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "42");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_EOF);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_EQ(entry->line, 1u);
		ASSERT_EQ(entry->col, 1u);
		ASSERT_STR_CONTAINS(entry->message, "Unexpected character '#', did you mean '#entrypoint'?");
	}

	TEST(StandaloneSlashReportsDiagnosticAndRecovers)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "/ 42";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 3);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_INVALID);
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_INTEGER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "42");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_EOF);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_EQ(entry->line, 1u);
		ASSERT_EQ(entry->col, 1u);
		ASSERT_STR_CONTAINS(entry->message, "Unexpected character '/'");
	}

	TEST(MultipleErrorsAreReported)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "@ # ! 42";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 5);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_INVALID);
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_INVALID);
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_INVALID);
		ASSERT_EQ(tokens.items[3].type, TOKEN_TYPE_INTEGER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[3]), "42");
		ASSERT_EQ(tokens.items[4].type, TOKEN_TYPE_EOF);
		ASSERT_EQ(diag.CapturedCount(), 3u);
	}

	TEST(ConsecutiveInvalidCharactersAreReported)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "@@@ 42";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 5);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_INVALID);
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_INVALID);
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_INVALID);
		ASSERT_EQ(tokens.items[3].type, TOKEN_TYPE_INTEGER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[3]), "42");
		ASSERT_EQ(tokens.items[4].type, TOKEN_TYPE_EOF);
		ASSERT_EQ(diag.CapturedCount(), 3u);
	}

	TEST(PeekAtOffsetReturnsInvalidForBadInput)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "a @ b";
		File file          = MakeTestFile(alloc, source);
		Lexer lexer{&file, &diag};

		ASSERT_STR_EQ(TokenLexeme(source, lexer.PeekTokenAtOffset(0)), "a");
		ASSERT_EQ(lexer.PeekTokenAtOffset(1).type, TOKEN_TYPE_INVALID);
		ASSERT_STR_EQ(TokenLexeme(source, lexer.PeekTokenAtOffset(2)), "b");
		ASSERT_EQ(lexer.PeekTokenAtOffset(3).type, TOKEN_TYPE_EOF);

		// After peeks, the primed token should still be 'a'.
		ASSERT_STR_EQ(TokenLexeme(source, lexer.PeekToken()), "a");
	}

	TEST(ErrorOnLaterLine)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "foo\nbar\n@ 42";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 5);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "foo");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "bar");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_INVALID);
		ASSERT_EQ(tokens.items[3].type, TOKEN_TYPE_INTEGER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[3]), "42");
		ASSERT_EQ(tokens.items[4].type, TOKEN_TYPE_EOF);

		ASSERT_EQ(diag.CapturedCount(), 1u);
		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_EQ(entry->line, 3u);
		ASSERT_EQ(entry->col, 1u);
		ASSERT_STR_CONTAINS(entry->message, "Unexpected character '@'");
	}

	TEST(UnterminatedCommentOpenedOnLaterLine)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source = "foo\nbar\n<* unterminated";
		TokenList tokens   = LexSource(alloc, source, &diag);

		ASSERT_FALSE(tokens.truncated);
		ASSERT_EQ(tokens.count, 3);
		ASSERT_EQ(tokens.items[0].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[0]), "foo");
		ASSERT_EQ(tokens.items[1].type, TOKEN_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(TokenLexeme(source, tokens.items[1]), "bar");
		ASSERT_EQ(tokens.items[2].type, TOKEN_TYPE_EOF);

		ASSERT_EQ(diag.CapturedCount(), 1u);
		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_EQ(entry->line, 3u);
		ASSERT_EQ(entry->col, 1u);
		ASSERT_STR_CONTAINS(entry->message, "Unterminated multi-line comment");
	}

	TestResults RunLexerTests()
	{
		ResetTestCounters();

		HeapAllocator heap;
		ArenaAllocator arena(&heap, Megabytes(4));

		ScopedTimer timer;

		printf("%sRunning lexer tests...%s\n", TestColor(ANSI_COLOR_BOLD), TestColor(ANSI_COLOR_RESET));

		PrintSection("Token categories");
		RUN_TEST(EmptyAndCommentOnlyInputs);
		RUN_TEST(SingleCharacterTokens);
		RUN_TEST(DoubleCharacterTokens);
		RUN_TEST(Identifiers);
		RUN_TEST(KeywordAndBuiltinTypeTokens);
		RUN_TEST(DirectiveToken);
		RUN_TEST(KeywordPrefixesRemainIdentifiers);
		RUN_TEST(IntegerLiterals);
		RUN_TEST(FloatingPointLiterals);
		RUN_TEST(ZeroFloatLiterals);

		PrintSection("Token sequences");
		RUN_TEST(MixedTokenSequence);
		RUN_TEST(DotFollowedByNonDigit);
		RUN_TEST(LineCommentBetweenTokens);
		RUN_TEST(MultiLineCommentBetweenTokens);
		RUN_TEST(NestedMultiLineCommentBetweenTokens);
		RUN_TEST(EmptyMultiLineComment);
		RUN_TEST(LineCommentAtEofWithoutNewline);
		RUN_TEST(MultiLineCommentEndingExactlyAtEof);
		RUN_TEST(MultiLineCommentIgnoresLooseSpecialChars);
		RUN_TEST(CarriageReturnNewlineSeparatesTokens);
		RUN_TEST(LoneCarriageReturnSeparatesTokens);
		RUN_TEST(TokensSeparatedByWhitespace);
		RUN_TEST(ValidTokenLineAndColumnTracking);

		PrintSection("Lexer state");
		RUN_TEST(PeekReturnsSameToken);
		RUN_TEST(EatThenPeekAdvances);
		RUN_TEST(EatAllReachesEof);
		RUN_TEST(EatTokenBeforePeekIsSafe);
		RUN_TEST(PeekAtOffsetDoesNotRequirePriming);
		RUN_TEST(PeekAtOffsetFromPrimedLexerDoesNotConsume);
		RUN_TEST(PeekAtOffsetFarPastEof);
		RUN_TEST(PeekAtOffsetOnEmptyStream);
		RUN_TEST(PeekAtOffsetOnConsumedStream);

		PrintSection("Diagnostics and recovery");
		RUN_TEST(UnterminatedMultiLineComment);
		RUN_TEST(UnterminatedCommentAfterToken);
		RUN_TEST(StandaloneBangReportsDiagnosticAndRecovers);
		RUN_TEST(UnrecognizedCharacterReportsDiagnosticAndRecovers);
		RUN_TEST(FloatLiteralMissingSuffixReportsDiagnostic);
		RUN_TEST(UnknownDirectiveReportsDiagnosticAndRecovers);
		RUN_TEST(HashWithoutDirectiveReportsDiagnosticAndRecovers);
		RUN_TEST(StandaloneSlashReportsDiagnosticAndRecovers);
		RUN_TEST(MultipleErrorsAreReported);
		RUN_TEST(ConsecutiveInvalidCharactersAreReported);
		RUN_TEST(PeekAtOffsetReturnsInvalidForBadInput);
		RUN_TEST(ErrorOnLaterLine);
		RUN_TEST(UnterminatedCommentOpenedOnLaterLine);
		RUN_TEST(LeadingZerosOnNumericLiteralsAreRejected);
		RUN_TEST(LeadingZeroDiagnosticsUseDisplayColumnsForTabs);
		RUN_TEST(DiagnosticRendererShowsSpanAtColumnOne);
		RUN_TEST(DiagnosticRendererExpandsTabsBeforeCaret);
		RUN_TEST(DiagnosticRendererClipsLongLinesOnTheRight);
		RUN_TEST(DiagnosticRendererClipsLongLinesOnBothSides);

		f64 totalMs = timer.GetElapsedMilliseconds();
		return PrintTestSummary("Lexer", totalMs);
	}

} // namespace Wandelt
