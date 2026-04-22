#include "Parser.hpp"

#include <charconv>

namespace Wandelt
{

	static Statement s_InvalidStmt   = {.type = STATEMENT_TYPE_INVALID};
	static Declaration s_InvalidDecl = {.type = DECLARATION_TYPE_INVALID};
	static Expression s_InvalidExpr  = {.type = EXPRESSION_TYPE_INVALID};

	static BuiltinTypeKind BuiltinTypeKindFromTokenType(TokenType type)
	{
		switch (type)
		{
		case TOKEN_TYPE_VOID_KEYWORD:
			return BUILTIN_TYPE_VOID;
		case TOKEN_TYPE_BOOL_KEYWORD:
			return BUILTIN_TYPE_BOOL;
		case TOKEN_TYPE_CHAR_KEYWORD:
			return BUILTIN_TYPE_CHAR;
		case TOKEN_TYPE_UCHAR_KEYWORD:
			return BUILTIN_TYPE_UCHAR;
		case TOKEN_TYPE_SHORT_KEYWORD:
			return BUILTIN_TYPE_SHORT;
		case TOKEN_TYPE_USHORT_KEYWORD:
			return BUILTIN_TYPE_USHORT;
		case TOKEN_TYPE_INT_KEYWORD:
			return BUILTIN_TYPE_INT;
		case TOKEN_TYPE_UINT_KEYWORD:
			return BUILTIN_TYPE_UINT;
		case TOKEN_TYPE_LONG_KEYWORD:
			return BUILTIN_TYPE_LONG;
		case TOKEN_TYPE_ULONG_KEYWORD:
			return BUILTIN_TYPE_ULONG;
		case TOKEN_TYPE_SZ_KEYWORD:
			return BUILTIN_TYPE_SZ;
		case TOKEN_TYPE_USZ_KEYWORD:
			return BUILTIN_TYPE_USZ;
		case TOKEN_TYPE_INTPTR_KEYWORD:
			return BUILTIN_TYPE_INTPTR;
		case TOKEN_TYPE_UINTPTR_KEYWORD:
			return BUILTIN_TYPE_UINTPTR;
		case TOKEN_TYPE_FLOAT_KEYWORD:
			return BUILTIN_TYPE_FLOAT;
		case TOKEN_TYPE_DOUBLE_KEYWORD:
			return BUILTIN_TYPE_DOUBLE;
		case TOKEN_TYPE_STRING_KEYWORD:
			return BUILTIN_TYPE_STRING;
		case TOKEN_TYPE_CSTRING_KEYWORD:
			return BUILTIN_TYPE_CSTRING;
		case TOKEN_TYPE_RAWPTR_KEYWORD:
			return BUILTIN_TYPE_RAWPTR;
		default:
			return BUILTIN_TYPE_INVALID;
		}
	}

	static bool IsBuiltinTypeKeyword(TokenType type)
	{
		return BuiltinTypeKindFromTokenType(type) != BUILTIN_TYPE_INVALID;
	}

	static bool IsStatementStarter(TokenType type)
	{
		switch (type)
		{
		case TOKEN_TYPE_PACKAGE_KEYWORD:
		case TOKEN_TYPE_RETURN_KEYWORD:
		case TOKEN_TYPE_FN_KEYWORD:
		case TOKEN_TYPE_IDENTIFIER:
			return true;
		default:
			return IsBuiltinTypeKeyword(type);
		}
	}

	Parser::Parser(Allocator* stmtAllocator, Allocator* declAllocator, Allocator* exprAllocator, Lexer* lexer, Diagnostics* diagnostics)
	    : m_StmtAllocator(stmtAllocator), m_DeclAllocator(declAllocator), m_ExprAllocator(exprAllocator), m_Lexer(lexer), m_Diagnostics(diagnostics)
	{
	}

	TranslationUnit Parser::Parse()
	{
		m_TranslationUnit = {.file = m_Lexer->GetFile(), .statements = Vector<Statement*>::Create(m_StmtAllocator, 10)};

		// Statement* lastStmt = nullptr;

		while (m_Lexer->PeekToken().type != TOKEN_TYPE_EOF)
		{
			Statement* stmt = ParseTopLevelStatement();
			ASSERT(stmt);

			if (stmt->type == STATEMENT_TYPE_INVALID)
				RecoverFromError(ParseScope::TopLevel);

			/*if (lastStmt != nullptr)
			    lastStmt->next = stmt;

			lastStmt = stmt;*/

			m_TranslationUnit.statements.Push(stmt);
		}

		return m_TranslationUnit;
	}

	Statement* Parser::ParseTopLevelStatement()
	{
		Token token = m_Lexer->PeekToken();

		switch (token.type)
		{
		case TOKEN_TYPE_PACKAGE_KEYWORD:
			return ParseDeclarationStatement();

		case TOKEN_TYPE_RETURN_KEYWORD:
			return ParseReturnStatement();

		case TOKEN_TYPE_FN_KEYWORD:
			return ParseDeclarationStatement();

		case TOKEN_TYPE_IDENTIFIER:
			return ParseExpressionStatement();

		default:
			if (IsBuiltinTypeKeyword(token.type))
				return ParseDeclarationStatement();

			m_Diagnostics->ReportError(token.span, m_Lexer->GetFile(), "Expected a top-level statement, but found '{}'", TokenTypeToCStr(token.type));
			break;
		}

		return &s_InvalidStmt;
	}

	Statement* Parser::ParseInnerStatement()
	{
		Token token = m_Lexer->PeekToken();

		switch (token.type)
		{
		case TOKEN_TYPE_PACKAGE_KEYWORD:
			return ParseDeclarationStatement();

		case TOKEN_TYPE_RETURN_KEYWORD:
			return ParseReturnStatement();

		case TOKEN_TYPE_FN_KEYWORD:
			return ParseDeclarationStatement();
		case TOKEN_TYPE_IDENTIFIER:
			return ParseExpressionStatement();

		default:
			if (IsBuiltinTypeKeyword(token.type))
				return ParseDeclarationStatement();

			m_Diagnostics->ReportError(token.span, m_Lexer->GetFile(), "Expected a statement, but found '{}'", TokenTypeToCStr(token.type));
			break;
		}

		return &s_InvalidStmt;
	}

	Statement* Parser::ParseDeclarationStatement()
	{
		Statement* stmt = m_StmtAllocator->Alloc<Statement>();
		stmt->type      = STATEMENT_TYPE_DECLARATION;

		stmt->declaration.declaration = ParseDeclaration();
		if (stmt->declaration.declaration->type == DECLARATION_TYPE_INVALID)
			return &s_InvalidStmt;

		stmt->span = stmt->declaration.declaration->span;

		return stmt;
	}

	Statement* Parser::ParseExpressionStatement()
	{
		Statement* stmt             = m_StmtAllocator->Alloc<Statement>();
		stmt->type                  = STATEMENT_TYPE_EXPRESSION;
		stmt->expression.expression = ParseExpression();

		if (stmt->expression.expression->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidStmt;

		const Token semicolonToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_SEMICOLON))
			return &s_InvalidStmt;

		stmt->span = Span::Extend(stmt->expression.expression->span, semicolonToken.span);

		return stmt;
	}

	Statement* Parser::ParseReturnStatement()
	{
		Statement* stmt = m_StmtAllocator->Alloc<Statement>();
		stmt->type      = STATEMENT_TYPE_RETURN;

		const Token returnToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_RETURN_KEYWORD))
			return &s_InvalidStmt;

		stmt->returnStmt.expression = ParseExpression();
		if (stmt->returnStmt.expression->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidStmt;

		const Token semicolonToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_SEMICOLON))
			return &s_InvalidStmt;

		stmt->span = Span::Extend(returnToken.span, semicolonToken.span);

		return stmt;
	}

	Statement* Parser::ParseBlockStatement()
	{
		const Token openBrace = m_Lexer->PeekToken();
		if (openBrace.type != TOKEN_TYPE_OPEN_BRACE)
		{
			m_Diagnostics->ReportError(openBrace.span, m_Lexer->GetFile(), "Expected a '{{' to start a scope, but got '{}'",
			                           TokenTypeToCStr(openBrace.type));
			return &s_InvalidStmt;
		}

		m_Lexer->EatToken();

		Statement* stmt = m_StmtAllocator->Alloc<Statement>();
		stmt->type      = STATEMENT_TYPE_BLOCK;

		stmt->block.statements = Vector<Statement*>::Create(m_StmtAllocator, 4);

		while (m_Lexer->PeekToken().type != TOKEN_TYPE_CLOSE_BRACE && m_Lexer->PeekToken().type != TOKEN_TYPE_EOF)
		{
			Statement* innerStmt = ParseInnerStatement();
			if (innerStmt->type == STATEMENT_TYPE_INVALID)
			{
				RecoverFromError(ParseScope::Block);
				continue;
			}

			stmt->block.statements.Push(innerStmt);
		}

		const Token closeBrace = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_CLOSE_BRACE))
			return &s_InvalidStmt;

		stmt->span = Span::Extend(openBrace.span, closeBrace.span);

		return stmt;
	}

	Declaration* Parser::ParseDeclaration()
	{
		Token token = m_Lexer->PeekToken();

		switch (token.type)
		{
		case TOKEN_TYPE_PACKAGE_KEYWORD:
			return ParsePackageDeclaration();

		case TOKEN_TYPE_FN_KEYWORD:
			return ParseFunctionDeclaration();

		default:
			if (IsBuiltinTypeKeyword(token.type))
				return ParseVariableDeclaration();

			m_Diagnostics->ReportError(token.span, m_Lexer->GetFile(), "Expected a declaration, but found '{}'", TokenTypeToCStr(token.type));
			break;
		}

		return &s_InvalidDecl;
	}

	Declaration* Parser::ParsePackageDeclaration()
	{
		const Token packageToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_PACKAGE_KEYWORD))
			return &s_InvalidDecl;

		Declaration* decl = m_DeclAllocator->Alloc<Declaration>();
		decl->type        = DECLARATION_TYPE_PACKAGE;

		if (!ParseIdentifier(&decl->package.name))
			return &s_InvalidDecl;

		while (true)
		{
			const Token directiveToken = m_Lexer->PeekToken();

			if (directiveToken.type == TOKEN_TYPE_ENTRYPOINT_DIRECTIVE)
			{
				if (decl->package.isEntrypoint)
				{
					m_Diagnostics->ReportError(directiveToken.span, m_Lexer->GetFile(), "Duplicate '#entrypoint' directive on package declaration");
					return &s_InvalidDecl;
				}

				decl->package.isEntrypoint = true;
				m_Lexer->EatToken();
				continue;
			}

			break;
		}

		const Token semicolonToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_SEMICOLON))
			return &s_InvalidDecl;

		decl->span = Span::Extend(packageToken.span, semicolonToken.span);

		return decl;
	}

	Declaration* Parser::ParseVariableDeclaration()
	{
		const Token startToken = m_Lexer->PeekToken();

		Declaration* decl = m_DeclAllocator->Alloc<Declaration>();
		decl->type        = DECLARATION_TYPE_VARIABLE;

		if (!ParseType(&decl->variable.type))
			return &s_InvalidDecl;

		if (!ParseIdentifier(&decl->variable.name))
			return &s_InvalidDecl;

		if (!ParseToken(TOKEN_TYPE_EQUALS))
			return &s_InvalidDecl;

		decl->variable.initializer = ParseExpression();
		if (decl->variable.initializer->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidDecl;

		const Token semicolonToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_SEMICOLON))
			return &s_InvalidDecl;

		decl->span = Span::Extend(startToken.span, semicolonToken.span);

		return decl;
	}

	Declaration* Parser::ParseFunctionDeclaration()
	{
		const Token fnToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_FN_KEYWORD))
			return &s_InvalidDecl;

		Declaration* decl = m_DeclAllocator->Alloc<Declaration>();
		decl->type        = DECLARATION_TYPE_FUNCTION;

		decl->function.parameters.m_Data = nullptr;

		if (!ParseType(&decl->function.returnType))
			return &s_InvalidDecl;

		if (!ParseIdentifier(&decl->function.name))
			return &s_InvalidDecl;

		if (!ParseToken(TOKEN_TYPE_OPEN_PAREN))
			return &s_InvalidDecl;

		if (!ParseToken(TOKEN_TYPE_CLOSE_PAREN))
			return &s_InvalidDecl;

		decl->function.body = ParseBlockStatement();
		if (decl->function.body->type == STATEMENT_TYPE_INVALID)
			return &s_InvalidDecl;

		decl->span = Span::Extend(fnToken.span, decl->function.body->span);

		return decl;
	}

	Expression* Parser::ParseExpression()
	{
		return ParseExpressionWithPrecedence(PRECEDENCE_NONE);
	}

	Expression* Parser::ParseExpressionWithPrecedence(Precedence minPrecedence)
	{
		Token token = m_Lexer->PeekToken();
		if (token.type == TOKEN_TYPE_INVALID)
			return &s_InvalidExpr;

		PrefixParseFn prefixRule = s_ParseRules[token.type].prefix;
		if (prefixRule == nullptr)
		{
			m_Diagnostics->ReportError(token.span, m_Lexer->GetFile(), "Expected an expression, but found '{}'", TokenTypeToCStr(token.type));
			return &s_InvalidExpr;
		}

		Expression* left = (this->*prefixRule)();
		if (left->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidExpr;

		if (m_Lexer->PeekToken().type == TOKEN_TYPE_INVALID)
			return &s_InvalidExpr;

		while (minPrecedence <= s_ParseRules[m_Lexer->PeekToken().type].precedence)
		{
			const Token infixToken = m_Lexer->PeekToken();

			InfixParseFn infixRule = s_ParseRules[infixToken.type].infix;
			if (infixRule == nullptr)
				break;

			left = (this->*infixRule)(left);
			if (left->type == EXPRESSION_TYPE_INVALID)
				return &s_InvalidExpr;

			if (m_Lexer->PeekToken().type == TOKEN_TYPE_INVALID)
				return &s_InvalidExpr;
		}

		return left;
	}

	Expression* Parser::ParseConstantExpression()
	{
		const Token token = m_Lexer->PeekToken();

		Expression* expr = m_ExprAllocator->Alloc<Expression>();
		expr->type       = EXPRESSION_TYPE_CONSTANT;
		expr->span       = token.span;

		const StringView lexeme = m_Lexer->GetFile()->GetViewOverPartOfContent(token.span.begin, token.span.end - token.span.begin);
		const char* first       = lexeme.Data();
		const char* last        = first + lexeme.Length();

		std::from_chars_result result = {};

		switch (token.type)
		{
		case TOKEN_TYPE_INTEGER:
			expr->constant.kind = CONSTANT_KIND_INTEGER;
			result              = std::from_chars(first, last, expr->constant.integerValue);
			break;

		case TOKEN_TYPE_FLOAT:
			expr->constant.kind = CONSTANT_KIND_FLOAT;
			result              = std::from_chars(first, last, expr->constant.floatValue);
			break;

		case TOKEN_TYPE_DOUBLE:
			expr->constant.kind = CONSTANT_KIND_DOUBLE;
			result              = std::from_chars(first, last, expr->constant.doubleValue);
			break;

		case TOKEN_TYPE_TRUE:
		case TOKEN_TYPE_FALSE:
			expr->constant.kind         = CONSTANT_KIND_BOOLEAN;
			expr->constant.booleanValue = (token.type == TOKEN_TYPE_TRUE);
			break;

		default:
			ASSERT(false, "unhandled constant token type");
			return &s_InvalidExpr;
		}

		const char* targetTypeName = "";
		switch (token.type)
		{
		case TOKEN_TYPE_INTEGER:
			targetTypeName = "integer";
			break;
		case TOKEN_TYPE_FLOAT:
			targetTypeName = "float";
			break;
		case TOKEN_TYPE_DOUBLE:
			targetTypeName = "double";
			break;
		default:
			break;
		}

		if (result.ec == std::errc::result_out_of_range)
		{
			m_Diagnostics->ReportError(token.span, m_Lexer->GetFile(), "Numeric literal '{}' does not fit in {}", lexeme, targetTypeName);
			return &s_InvalidExpr;
		}

		if (result.ec == std::errc::invalid_argument)
		{
			m_Diagnostics->ReportError(token.span, m_Lexer->GetFile(), "Malformed numeric literal '{}' (could not parse as {})", lexeme,
			                           targetTypeName);
			return &s_InvalidExpr;
		}

		m_Lexer->EatToken();

		return expr;
	}

	Expression* Parser::ParseIdentifierExpression()
	{
		const Token token = m_Lexer->PeekToken();

		Expression* expr = m_ExprAllocator->Alloc<Expression>();
		expr->type       = EXPRESSION_TYPE_IDENTIFIER;
		expr->span       = token.span;

		if (!ParseIdentifier(&expr->identifier.name))
			return &s_InvalidExpr;

		return expr;
	}

	Expression* Parser::ParseCastExpression()
	{
		const Token asToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_CAST_KEYWORD))
			return &s_InvalidExpr;

		if (!ParseToken(TOKEN_TYPE_OPEN_PAREN))
			return &s_InvalidExpr;

		Type* targetType = nullptr;
		if (!ParseType(&targetType))
			return &s_InvalidExpr;

		if (!ParseToken(TOKEN_TYPE_CLOSE_PAREN))
			return &s_InvalidExpr;

		// Parse the operand at POSTFIX precedence so `cast(T) foo()` parses as `cast(T)(foo())`
		Expression* operand = ParseExpressionWithPrecedence(PRECEDENCE_POSTFIX);
		if (operand->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidExpr;

		Expression* expr      = m_ExprAllocator->Alloc<Expression>();
		expr->type            = EXPRESSION_TYPE_CAST;
		expr->span            = Span::Extend(asToken.span, operand->span);
		expr->cast.targetType = targetType;
		expr->cast.expression = operand;

		return expr;
	}

	Expression* Parser::ParseCallExpression(Expression* left)
	{
		if (left->type != EXPRESSION_TYPE_IDENTIFIER)
		{
			m_Diagnostics->ReportError(left->span, m_Lexer->GetFile(), "Call target must be an identifier, but got a '{}' expression",
			                           ExpressionTypeToCStr(left->type));
			return &s_InvalidExpr;
		}

		if (!ParseToken(TOKEN_TYPE_OPEN_PAREN))
			return &s_InvalidExpr;

		Expression* expr        = m_ExprAllocator->Alloc<Expression>();
		expr->type              = EXPRESSION_TYPE_CALL;
		expr->call.functionName = left->identifier.name;

		const Token closeParen = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_CLOSE_PAREN))
			return &s_InvalidExpr;

		expr->span = Span::Extend(left->span, closeParen.span);

		return expr;
	}

	void Parser::RecoverFromError(ParseScope scope)
	{
		i32 braceDepth = 0;
		i32 parenDepth = 0;

		while (true)
		{
			const TokenType type = m_Lexer->PeekToken().type;

			if (type == TOKEN_TYPE_EOF)
				return;

			if (braceDepth > 0 || parenDepth > 0)
			{
				switch (type)
				{
				case TOKEN_TYPE_OPEN_BRACE:
					braceDepth++;
					break;
				case TOKEN_TYPE_CLOSE_BRACE:
					if (braceDepth > 0)
						braceDepth--;
					break;
				case TOKEN_TYPE_OPEN_PAREN:
					parenDepth++;
					break;
				case TOKEN_TYPE_CLOSE_PAREN:
					if (parenDepth > 0)
						parenDepth--;
					break;
				default:
					break;
				}
				m_Lexer->EatToken();
				continue;
			}

			switch (type)
			{
			case TOKEN_TYPE_SEMICOLON:
				m_Lexer->EatToken();
				return;

			case TOKEN_TYPE_OPEN_BRACE:
				braceDepth++;
				m_Lexer->EatToken();
				continue;

			case TOKEN_TYPE_OPEN_PAREN:
				parenDepth++;
				m_Lexer->EatToken();
				continue;

			case TOKEN_TYPE_CLOSE_BRACE:
				// In a block: leave it for the enclosing ParseBlockStatement to consume.
				// At top level: stray brace — eat it and keep scanning.
				if (scope == ParseScope::Block)
					return;
				m_Lexer->EatToken();
				continue;

			default:
				break;
			}

			if (IsStatementStarter(type))
				return;

			m_Lexer->EatToken();
		}
	}

	bool Parser::ParseToken(TokenType expectedType)
	{
		Token token = m_Lexer->PeekToken();

		if (token.type != expectedType)
		{
			m_Diagnostics->ReportError(token.span, m_Lexer->GetFile(), "Expected a '{}', but got '{}'", TokenTypeToCStr(expectedType),
			                           TokenTypeToCStr(token.type));

			return false;
		}

		m_Lexer->EatToken();

		return true;
	}

	bool Parser::ParseIdentifier(StringView* outIdentifier)
	{
		Token token = m_Lexer->PeekToken();

		if (token.type != TOKEN_TYPE_IDENTIFIER)
		{
			m_Diagnostics->ReportError(token.span, m_Lexer->GetFile(), "Expected an identifier, but got '{}'", TokenTypeToCStr(token.type));
			return false;
		}

		StringView part = m_Lexer->GetFile()->GetViewOverPartOfContent(token.span.begin, token.span.end - token.span.begin);

		*outIdentifier = part;

		m_Lexer->EatToken();

		return true;
	}

	bool Parser::ParseType(Type** outType)
	{
		Token token = m_Lexer->PeekToken();

		BuiltinTypeKind builtinType = BuiltinTypeKindFromTokenType(token.type);
		if (builtinType == BUILTIN_TYPE_INVALID)
		{
			m_Diagnostics->ReportError(token.span, m_Lexer->GetFile(), "Expected a type, but got '{}'", TokenTypeToCStr(token.type));
			return false;
		}

		Type* type = Type::GetBuiltinType(builtinType);
		*outType   = type;

		m_Lexer->EatToken();

		return true;
	}

	// Rows = token type, each row = {prefix, infix, precedence}.
	const ParseRule Parser::s_ParseRules[TOKEN_TYPE_COUNT] = {
	    /* INVALID              */ {nullptr, nullptr, PRECEDENCE_NONE},

	    /* PACKAGE_KEYWORD      */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* RETURN_KEYWORD       */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* CAST_KEYWORD         */ {&Parser::ParseCastExpression, nullptr, PRECEDENCE_NONE},
	    /* FN_KEYWORD           */ {nullptr, nullptr, PRECEDENCE_NONE},

	    /* ENTRYPOINT_DIRECTIVE */ {nullptr, nullptr, PRECEDENCE_NONE},

	    /* VOID_KEYWORD         */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* BOOL_KEYWORD         */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* CHAR_KEYWORD         */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* UCHAR_KEYWORD        */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* SHORT_KEYWORD        */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* USHORT_KEYWORD       */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* INT_KEYWORD          */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* UINT_KEYWORD         */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* LONG_KEYWORD         */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* ULONG_KEYWORD        */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* SZ_KEYWORD           */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* USZ_KEYWORD          */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* INTPTR_KEYWORD       */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* UINTPTR_KEYWORD      */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* FLOAT_KEYWORD        */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* DOUBLE_KEYWORD       */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* STRING_KEYWORD       */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* CSTRING_KEYWORD      */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* RAWPTR_KEYWORD       */ {nullptr, nullptr, PRECEDENCE_NONE},

	    /* OPEN_PAREN           */ {nullptr, &Parser::ParseCallExpression, PRECEDENCE_POSTFIX},
	    /* CLOSE_PAREN          */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* OPEN_BRACE           */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* CLOSE_BRACE          */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* SEMICOLON            */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* EQUALS               */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* DOT                  */ {nullptr, nullptr, PRECEDENCE_NONE},

	    /* BANG_BANG            */ {nullptr, nullptr, PRECEDENCE_NONE},

	    /* IDENTIFIER           */ {&Parser::ParseIdentifierExpression, nullptr, PRECEDENCE_NONE},
	    /* INTEGER              */ {&Parser::ParseConstantExpression, nullptr, PRECEDENCE_NONE},
	    /* FLOAT                */ {&Parser::ParseConstantExpression, nullptr, PRECEDENCE_NONE},
	    /* DOUBLE               */ {&Parser::ParseConstantExpression, nullptr, PRECEDENCE_NONE},
	    /* TRUE                 */ {&Parser::ParseConstantExpression, nullptr, PRECEDENCE_NONE},
	    /* FALSE                */ {&Parser::ParseConstantExpression, nullptr, PRECEDENCE_NONE},

	    /* EOF                  */ {nullptr, nullptr, PRECEDENCE_NONE},
	};

} // namespace Wandelt
