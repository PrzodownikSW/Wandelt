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

	static bool IsAssignmentToken(TokenType type)
	{
		switch (type)
		{
		case TOKEN_TYPE_EQUALS:
		case TOKEN_TYPE_PLUS_EQUAL:
		case TOKEN_TYPE_MINUS_EQUAL:
		case TOKEN_TYPE_STAR_EQUAL:
		case TOKEN_TYPE_SLASH_EQUAL:
			return true;
		default:
			return false;
		}
	}

	static bool IsBinaryOperatorToken(TokenType type)
	{
		switch (type)
		{
		case TOKEN_TYPE_PLUS:
		case TOKEN_TYPE_MINUS:
		case TOKEN_TYPE_STAR:
		case TOKEN_TYPE_SLASH:
		case TOKEN_TYPE_EQUAL_EQUAL:
		case TOKEN_TYPE_BANG_EQUAL:
		case TOKEN_TYPE_LESS:
		case TOKEN_TYPE_GREATER:
		case TOKEN_TYPE_LESS_EQUAL:
		case TOKEN_TYPE_GREATER_EQUAL:
			return true;
		default:
			return false;
		}
	}

	static bool IsIncDecToken(TokenType type)
	{
		return type == TOKEN_TYPE_PLUS_PLUS || type == TOKEN_TYPE_MINUS_MINUS;
	}

	static Precedence GetNextHigherPrecedence(Precedence precedence)
	{
		ASSERT(precedence < PRECEDENCE_PRIMARY);
		return static_cast<Precedence>(static_cast<u32>(precedence) + 1u);
	}

	static bool IsTopLevelOnlyDeclarationStarter(TokenType type)
	{
		switch (type)
		{
		case TOKEN_TYPE_PACKAGE_KEYWORD:
		case TOKEN_TYPE_FN_KEYWORD:
			return true;
		default:
			return false;
		}
	}

	static bool IsInnerStatementStarter(TokenType type)
	{
		switch (type)
		{
		case TOKEN_TYPE_RETURN_KEYWORD:
		case TOKEN_TYPE_DISCARD_KEYWORD:
		case TOKEN_TYPE_IF_KEYWORD:
		case TOKEN_TYPE_WHILE_KEYWORD:
		case TOKEN_TYPE_FOR_KEYWORD:
		case TOKEN_TYPE_BREAK_KEYWORD:
		case TOKEN_TYPE_CONTINUE_KEYWORD:
		case TOKEN_TYPE_IDENTIFIER:
		case TOKEN_TYPE_PLUS_PLUS:
		case TOKEN_TYPE_MINUS_MINUS:
			return true;
		default:
			return IsBuiltinTypeKeyword(type);
		}
	}

	static bool IsTopLevelStatementStarter(TokenType type)
	{
		return IsInnerStatementStarter(type) || IsTopLevelOnlyDeclarationStarter(type);
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

		case TOKEN_TYPE_IF_KEYWORD:
			return ParseIfStatement();

		case TOKEN_TYPE_WHILE_KEYWORD:
			return ParseWhileStatement();

		case TOKEN_TYPE_FOR_KEYWORD:
			return ParseForStatement();

		case TOKEN_TYPE_BREAK_KEYWORD:
			return ParseBreakStatement();

		case TOKEN_TYPE_CONTINUE_KEYWORD:
			return ParseContinueStatement();

		case TOKEN_TYPE_DISCARD_KEYWORD:
		case TOKEN_TYPE_IDENTIFIER:
		case TOKEN_TYPE_PLUS_PLUS:
		case TOKEN_TYPE_MINUS_MINUS:
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
		case TOKEN_TYPE_RETURN_KEYWORD:
			return ParseReturnStatement();

		case TOKEN_TYPE_IF_KEYWORD:
			return ParseIfStatement();

		case TOKEN_TYPE_WHILE_KEYWORD:
			return ParseWhileStatement();

		case TOKEN_TYPE_FOR_KEYWORD:
			return ParseForStatement();

		case TOKEN_TYPE_BREAK_KEYWORD:
			return ParseBreakStatement();

		case TOKEN_TYPE_CONTINUE_KEYWORD:
			return ParseContinueStatement();

		case TOKEN_TYPE_DISCARD_KEYWORD:
		case TOKEN_TYPE_IDENTIFIER:
		case TOKEN_TYPE_PLUS_PLUS:
		case TOKEN_TYPE_MINUS_MINUS:
			return ParseExpressionStatement();

		default:
			if (IsBuiltinTypeKeyword(token.type))
				return ParseDeclarationStatement();

			// A top-level-only declaration misplaced inside a block: emit a single diagnostic and
			// let ParseDeclaration consume the whole construct so the rest of the block can parse
			// without cascading errors.
			if (IsTopLevelOnlyDeclarationStarter(token.type))
			{
				m_Diagnostics->ReportError(token.span, m_Lexer->GetFile(), "'{}' declarations are only allowed at the top level",
				                           TokenTypeToCStr(token.type));
				ParseDeclaration();
				return &s_InvalidStmt;
			}

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
		Statement* stmt            = m_StmtAllocator->Alloc<Statement>();
		stmt->type                 = STATEMENT_TYPE_EXPRESSION;
		stmt->expression.discarded = false;

		const Token firstToken = m_Lexer->PeekToken();
		Token startToken       = firstToken;

		if (firstToken.type == TOKEN_TYPE_DISCARD_KEYWORD)
		{
			stmt->expression.discarded = true;
			m_Lexer->EatToken();
		}

		stmt->expression.expression = ParseExpression();

		if (stmt->expression.expression->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidStmt;

		const Token semicolonToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_SEMICOLON))
			return &s_InvalidStmt;

		stmt->span = Span::Extend(startToken.span, semicolonToken.span);

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

	Statement* Parser::ParseIfStatement()
	{
		const Token ifToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_IF_KEYWORD))
			return &s_InvalidStmt;

		Statement* stmt         = m_StmtAllocator->Alloc<Statement>();
		stmt->type              = STATEMENT_TYPE_IF;
		stmt->ifStmt.elseBranch = nullptr;

		stmt->ifStmt.condition = ParseExpression();
		if (stmt->ifStmt.condition->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidStmt;

		stmt->ifStmt.thenBlock = ParseBlockStatement();
		if (stmt->ifStmt.thenBlock->type == STATEMENT_TYPE_INVALID)
			return &s_InvalidStmt;

		Span endSpan = stmt->ifStmt.thenBlock->span;

		if (m_Lexer->PeekToken().type == TOKEN_TYPE_ELSE_KEYWORD)
		{
			m_Lexer->EatToken();

			if (m_Lexer->PeekToken().type == TOKEN_TYPE_IF_KEYWORD)
				stmt->ifStmt.elseBranch = ParseIfStatement();
			else
				stmt->ifStmt.elseBranch = ParseBlockStatement();

			if (stmt->ifStmt.elseBranch->type == STATEMENT_TYPE_INVALID)
				return &s_InvalidStmt;

			endSpan = stmt->ifStmt.elseBranch->span;
		}

		stmt->span = Span::Extend(ifToken.span, endSpan);

		return stmt;
	}

	Statement* Parser::ParseWhileStatement()
	{
		const Token whileToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_WHILE_KEYWORD))
			return &s_InvalidStmt;

		Statement* stmt = m_StmtAllocator->Alloc<Statement>();
		stmt->type      = STATEMENT_TYPE_WHILE;

		stmt->whileStmt.condition = ParseExpression();
		if (stmt->whileStmt.condition->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidStmt;

		stmt->whileStmt.body = ParseBlockStatement();
		if (stmt->whileStmt.body->type == STATEMENT_TYPE_INVALID)
			return &s_InvalidStmt;

		stmt->span = Span::Extend(whileToken.span, stmt->whileStmt.body->span);

		return stmt;
	}

	Statement* Parser::ParseForStatement()
	{
		const Token forToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_FOR_KEYWORD))
			return &s_InvalidStmt;

		Statement* stmt = m_StmtAllocator->Alloc<Statement>();
		stmt->type      = STATEMENT_TYPE_FOR;

		// init: variable declaration or expression statement; both consume their own trailing ';'.
		const Token initToken = m_Lexer->PeekToken();
		if (IsBuiltinTypeKeyword(initToken.type))
			stmt->forStmt.init = ParseDeclarationStatement();
		else
			stmt->forStmt.init = ParseExpressionStatement();

		if (stmt->forStmt.init->type == STATEMENT_TYPE_INVALID)
			return &s_InvalidStmt;

		stmt->forStmt.condition = ParseExpression();
		if (stmt->forStmt.condition->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidStmt;

		if (!ParseToken(TOKEN_TYPE_SEMICOLON))
			return &s_InvalidStmt;

		stmt->forStmt.increment = ParseExpression();
		if (stmt->forStmt.increment->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidStmt;

		stmt->forStmt.body = ParseBlockStatement();
		if (stmt->forStmt.body->type == STATEMENT_TYPE_INVALID)
			return &s_InvalidStmt;

		stmt->span = Span::Extend(forToken.span, stmt->forStmt.body->span);

		return stmt;
	}

	Statement* Parser::ParseBreakStatement()
	{
		const Token breakToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_BREAK_KEYWORD))
			return &s_InvalidStmt;

		const Token semicolonToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_SEMICOLON))
			return &s_InvalidStmt;

		Statement* stmt = m_StmtAllocator->Alloc<Statement>();
		stmt->type      = STATEMENT_TYPE_BREAK;
		stmt->span      = Span::Extend(breakToken.span, semicolonToken.span);

		return stmt;
	}

	Statement* Parser::ParseContinueStatement()
	{
		const Token continueToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_CONTINUE_KEYWORD))
			return &s_InvalidStmt;

		const Token semicolonToken = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_SEMICOLON))
			return &s_InvalidStmt;

		Statement* stmt = m_StmtAllocator->Alloc<Statement>();
		stmt->type      = STATEMENT_TYPE_CONTINUE;
		stmt->span      = Span::Extend(continueToken.span, semicolonToken.span);

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

		if (m_Lexer->PeekToken().type != TOKEN_TYPE_CLOSE_PAREN)
		{
			decl->function.parameters = Vector<Declaration*>::Create(m_DeclAllocator, 4);

			while (true)
			{
				Declaration* parameter = nullptr;
				if (!ParseFunctionParameter(&parameter))
					return &s_InvalidDecl;

				decl->function.parameters.Push(parameter);

				if (m_Lexer->PeekToken().type != TOKEN_TYPE_COMMA)
					break;

				if (!ParseToken(TOKEN_TYPE_COMMA))
					return &s_InvalidDecl;
			}
		}

		if (!ParseToken(TOKEN_TYPE_CLOSE_PAREN))
			return &s_InvalidDecl;

		decl->function.body = ParseBlockStatement();
		if (decl->function.body->type == STATEMENT_TYPE_INVALID)
			return &s_InvalidDecl;

		decl->span = Span::Extend(fnToken.span, decl->function.body->span);

		return decl;
	}

	bool Parser::ParseFunctionParameter(Declaration** outParameter)
	{
		const Token startToken = m_Lexer->PeekToken();

		Declaration* parameter = m_DeclAllocator->Alloc<Declaration>();
		parameter->type        = DECLARATION_TYPE_VARIABLE;

		if (!ParseType(&parameter->variable.type))
			return false;

		const Token nameToken = m_Lexer->PeekToken();
		if (!ParseIdentifier(&parameter->variable.name))
			return false;

		parameter->span = Span::Extend(startToken.span, nameToken.span);
		*outParameter   = parameter;
		return true;
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

			// Consume non-structural tokens after diagnosing them here so recovery
			// does not reinterpret the same token as the start of a new statement.
			if (token.type != TOKEN_TYPE_SEMICOLON && token.type != TOKEN_TYPE_OPEN_BRACE && token.type != TOKEN_TYPE_CLOSE_PAREN &&
			    token.type != TOKEN_TYPE_CLOSE_BRACE && token.type != TOKEN_TYPE_EOF)
			{
				m_Lexer->EatToken();
			}

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

		case TOKEN_TYPE_CHARACTER: {
			expr->constant.kind = CONSTANT_KIND_CHAR;
			const char* data    = lexeme.Data();

			ASSERT(lexeme.Length() >= 3, "Character literal is too short");
			ASSERT(data[0] == '\'', "Character literal must start with a single quote");
			ASSERT(data[lexeme.Length() - 1] == '\'', "Character literal must end with a single quote");

			if (data[1] != '\\')
			{
				expr->constant.charValue = data[1];
			}
			else
			{
				switch (data[2])
				{
				case 'n':
					expr->constant.charValue = '\n';
					break;
				case 'r':
					expr->constant.charValue = '\r';
					break;
				case 't':
					expr->constant.charValue = '\t';
					break;
				case '\\':
					expr->constant.charValue = '\\';
					break;
				case '\'':
					expr->constant.charValue = '\'';
					break;
				case '0':
					expr->constant.charValue = '\0';
					break;
				default:
					ASSERT(false, "Unhandled character escape sequence");
					break;
				}
			}

			break;
		}

		case TOKEN_TYPE_STRING:
			expr->constant.kind        = CONSTANT_KIND_STRING;
			expr->constant.stringValue = StringView{lexeme.Data() + 1, lexeme.Length() - 2};
			break;

		default:
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

	Expression* Parser::ParseUnaryExpression()
	{
		const Token operatorToken = m_Lexer->PeekToken();

		UnaryOperator op = UNARY_OPERATOR_INVALID;
		switch (operatorToken.type)
		{
		case TOKEN_TYPE_MINUS:
			op = UNARY_OPERATOR_NEGATE;
			break;

		default:
			m_Diagnostics->ReportError(operatorToken.span, m_Lexer->GetFile(), "Expected a unary operator, but found '{}'",
			                           TokenTypeToCStr(operatorToken.type));
			return &s_InvalidExpr;
		}

		m_Lexer->EatToken();

		Expression* operand = ParseExpressionWithPrecedence(PRECEDENCE_PREFIX);
		if (operand->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidExpr;

		Expression* expr = m_ExprAllocator->Alloc<Expression>();
		expr->type       = EXPRESSION_TYPE_UNARY;
		expr->span       = Span::Extend(operatorToken.span, operand->span);

		expr->unary.op      = op;
		expr->unary.operand = operand;

		return expr;
	}

	Expression* Parser::ParseBinaryExpression(Expression* left)
	{
		ASSERT(left != nullptr);

		const Token operatorToken = m_Lexer->PeekToken();
		if (!IsBinaryOperatorToken(operatorToken.type))
		{
			m_Diagnostics->ReportError(operatorToken.span, m_Lexer->GetFile(), "Expected a binary operator, but found '{}'",
			                           TokenTypeToCStr(operatorToken.type));
			return &s_InvalidExpr;
		}

		const Precedence precedence = s_ParseRules[operatorToken.type].precedence;
		m_Lexer->EatToken();

		Expression* right = ParseExpressionWithPrecedence(GetNextHigherPrecedence(precedence));
		if (right->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidExpr;

		Expression* expr = m_ExprAllocator->Alloc<Expression>();
		expr->type       = EXPRESSION_TYPE_BINARY;
		expr->span       = Span::Extend(left->span, right->span);

		expr->binary.op    = TokenTypeToBinaryOperator(operatorToken.type);
		expr->binary.left  = left;
		expr->binary.right = right;

		return expr;
	}

	Expression* Parser::ParseGroupExpression()
	{
		const Token openParen = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_OPEN_PAREN))
			return &s_InvalidExpr;

		Expression* inner = ParseExpression();
		if (inner->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidExpr;

		const Token closeParen = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_CLOSE_PAREN))
			return &s_InvalidExpr;

		Expression* expr = m_ExprAllocator->Alloc<Expression>();
		expr->type       = EXPRESSION_TYPE_GROUP;
		expr->span       = Span::Extend(openParen.span, closeParen.span);

		expr->group.inner = inner;

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

	Expression* Parser::ParsePrefixIncDecExpression()
	{
		const Token operatorToken = m_Lexer->PeekToken();
		if (!IsIncDecToken(operatorToken.type))
		{
			m_Diagnostics->ReportError(operatorToken.span, m_Lexer->GetFile(), "Expected '++' or '--', but found '{}'",
			                           TokenTypeToCStr(operatorToken.type));
			return &s_InvalidExpr;
		}

		m_Lexer->EatToken();

		Expression* operand = ParseExpressionWithPrecedence(PRECEDENCE_PREFIX);
		if (operand->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidExpr;

		Expression* expr = m_ExprAllocator->Alloc<Expression>();
		expr->type       = EXPRESSION_TYPE_INCDEC;
		expr->span       = Span::Extend(operatorToken.span, operand->span);

		expr->incdec.operand     = operand;
		expr->incdec.isIncrement = operatorToken.type == TOKEN_TYPE_PLUS_PLUS;
		expr->incdec.isPostfix   = false;

		return expr;
	}

	Expression* Parser::ParsePostfixIncDecExpression(Expression* left)
	{
		ASSERT(left != nullptr);

		const Token operatorToken = m_Lexer->PeekToken();
		if (!IsIncDecToken(operatorToken.type))
		{
			m_Diagnostics->ReportError(operatorToken.span, m_Lexer->GetFile(), "Expected '++' or '--', but found '{}'",
			                           TokenTypeToCStr(operatorToken.type));
			return &s_InvalidExpr;
		}

		m_Lexer->EatToken();

		Expression* expr = m_ExprAllocator->Alloc<Expression>();
		expr->type       = EXPRESSION_TYPE_INCDEC;
		expr->span       = Span::Extend(left->span, operatorToken.span);

		expr->incdec.operand     = left;
		expr->incdec.isIncrement = operatorToken.type == TOKEN_TYPE_PLUS_PLUS;
		expr->incdec.isPostfix   = true;

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

		if (m_Lexer->PeekToken().type != TOKEN_TYPE_CLOSE_PAREN)
		{
			expr->call.arguments = Vector<CallExpression::Argument>::Create(m_ExprAllocator, 4);

			bool sawNamedArgument      = false;
			bool sawPositionalArgument = false;

			while (true)
			{
				CallExpression::Argument argument = {};
				bool isNamedArgument              = false;
				if (!ParseCallArgument(&argument, &isNamedArgument))
					return &s_InvalidExpr;

				if (isNamedArgument)
				{
					if (sawPositionalArgument)
					{
						m_Diagnostics->ReportError(argument.span, m_Lexer->GetFile(), "Cannot mix positional and named arguments in the same call");
						return &s_InvalidExpr;
					}

					sawNamedArgument = true;
				}
				else
				{
					if (sawNamedArgument)
					{
						m_Diagnostics->ReportError(argument.span, m_Lexer->GetFile(), "Cannot mix positional and named arguments in the same call");
						return &s_InvalidExpr;
					}

					sawPositionalArgument = true;
				}

				expr->call.arguments.Push(argument);

				if (m_Lexer->PeekToken().type != TOKEN_TYPE_COMMA)
					break;

				if (!ParseToken(TOKEN_TYPE_COMMA))
					return &s_InvalidExpr;
			}
		}

		const Token closeParen = m_Lexer->PeekToken();
		if (!ParseToken(TOKEN_TYPE_CLOSE_PAREN))
			return &s_InvalidExpr;

		expr->span = Span::Extend(left->span, closeParen.span);

		return expr;
	}

	bool Parser::ParseCallArgument(CallExpression::Argument* outArgument, bool* outIsNamed)
	{
		ASSERT(outArgument != nullptr);
		ASSERT(outIsNamed != nullptr);

		*outIsNamed = false;

		if (m_Lexer->PeekToken().type == TOKEN_TYPE_IDENTIFIER && m_Lexer->PeekTokenAtOffset(1).type == TOKEN_TYPE_EQUALS)
		{
			const Token nameToken = m_Lexer->PeekToken();
			if (!ParseIdentifier(&outArgument->name))
				return false;

			if (!ParseToken(TOKEN_TYPE_EQUALS))
				return false;

			outArgument->expression = ParseExpression();
			if (outArgument->expression->type == EXPRESSION_TYPE_INVALID)
				return false;

			outArgument->span = Span::Extend(nameToken.span, outArgument->expression->span);
			*outIsNamed       = true;
			return true;
		}

		outArgument->expression = ParseExpression();
		if (outArgument->expression->type == EXPRESSION_TYPE_INVALID)
			return false;

		outArgument->span = outArgument->expression->span;
		return true;
	}

	Expression* Parser::ParseAssignmentExpression(Expression* left)
	{
		ASSERT(left != nullptr);

		const Token operatorToken = m_Lexer->PeekToken();
		if (!IsAssignmentToken(operatorToken.type))
			return left;

		m_Lexer->EatToken();

		Expression* right = ParseExpressionWithPrecedence(PRECEDENCE_ASSIGNMENT);
		if (right->type == EXPRESSION_TYPE_INVALID)
			return &s_InvalidExpr;

		Expression* expr = m_ExprAllocator->Alloc<Expression>();
		expr->type       = EXPRESSION_TYPE_ASSIGNMENT;
		expr->span       = Span::Extend(left->span, right->span);

		expr->assignment.op    = TokenTypeToAssignmentOperator(operatorToken.type);
		expr->assignment.left  = left;
		expr->assignment.right = right;

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

			const bool isStarter = (scope == ParseScope::Block) ? IsInnerStatementStarter(type) : IsTopLevelStatementStarter(type);
			if (isStarter)
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

	static_assert(TOKEN_TYPE_COUNT == 68,
	              "If you add a new token type, make sure to update the parse rules table below IN PROPER ORDER otherwise all hell will break loose");

	// Rows = token type, each row = {prefix, infix, precedence}.
	const ParseRule Parser::s_ParseRules[TOKEN_TYPE_COUNT] = {
	    /* INVALID              */ {nullptr, nullptr, PRECEDENCE_NONE},

	    /* PACKAGE_KEYWORD      */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* RETURN_KEYWORD       */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* CAST_KEYWORD         */ {&Parser::ParseCastExpression, nullptr, PRECEDENCE_NONE},
	    /* FN_KEYWORD           */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* DISCARD_KEYWORD      */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* IF_KEYWORD           */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* ELSE_KEYWORD         */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* WHILE_KEYWORD        */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* FOR_KEYWORD          */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* BREAK_KEYWORD        */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* CONTINUE_KEYWORD     */ {nullptr, nullptr, PRECEDENCE_NONE},

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

	    /* OPEN_PAREN           */ {&Parser::ParseGroupExpression, &Parser::ParseCallExpression, PRECEDENCE_POSTFIX},
	    /* CLOSE_PAREN          */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* OPEN_BRACE           */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* CLOSE_BRACE          */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* SEMICOLON            */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* COMMA                */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* DOT                  */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* EQUALS               */ {nullptr, &Parser::ParseAssignmentExpression, PRECEDENCE_ASSIGNMENT},
	    /* PLUS                 */ {nullptr, &Parser::ParseBinaryExpression, PRECEDENCE_ADDITIVE},
	    /* MINUS                */ {&Parser::ParseUnaryExpression, &Parser::ParseBinaryExpression, PRECEDENCE_ADDITIVE},
	    /* STAR                 */ {nullptr, &Parser::ParseBinaryExpression, PRECEDENCE_MULTIPLICATIVE},
	    /* SLASH                */ {nullptr, &Parser::ParseBinaryExpression, PRECEDENCE_MULTIPLICATIVE},
	    /* GREATER              */ {nullptr, &Parser::ParseBinaryExpression, PRECEDENCE_COMPARISON},
	    /* LESS                 */ {nullptr, &Parser::ParseBinaryExpression, PRECEDENCE_COMPARISON},
	    /* SINGLE_QUOTE         */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* DOUBLE_QUOTE         */ {nullptr, nullptr, PRECEDENCE_NONE},

	    /* BANG_BANG            */ {nullptr, nullptr, PRECEDENCE_NONE},
	    /* GREATER_EQUAL        */ {nullptr, &Parser::ParseBinaryExpression, PRECEDENCE_COMPARISON},
	    /* LESS_EQUAL           */ {nullptr, &Parser::ParseBinaryExpression, PRECEDENCE_COMPARISON},
	    /* EQUAL_EQUAL          */ {nullptr, &Parser::ParseBinaryExpression, PRECEDENCE_EQUALITY},
	    /* BANG_EQUAL           */ {nullptr, &Parser::ParseBinaryExpression, PRECEDENCE_EQUALITY},
	    /* PLUS_EQUAL           */ {nullptr, &Parser::ParseAssignmentExpression, PRECEDENCE_ASSIGNMENT},
	    /* MINUS_EQUAL          */ {nullptr, &Parser::ParseAssignmentExpression, PRECEDENCE_ASSIGNMENT},
	    /* STAR_EQUAL           */ {nullptr, &Parser::ParseAssignmentExpression, PRECEDENCE_ASSIGNMENT},
	    /* SLASH_EQUAL          */ {nullptr, &Parser::ParseAssignmentExpression, PRECEDENCE_ASSIGNMENT},
	    /* PLUS_PLUS            */ {&Parser::ParsePrefixIncDecExpression, &Parser::ParsePostfixIncDecExpression, PRECEDENCE_POSTFIX},
	    /* MINUS_MINUS          */ {&Parser::ParsePrefixIncDecExpression, &Parser::ParsePostfixIncDecExpression, PRECEDENCE_POSTFIX},

	    /* IDENTIFIER           */ {&Parser::ParseIdentifierExpression, nullptr, PRECEDENCE_NONE},
	    /* INTEGER              */ {&Parser::ParseConstantExpression, nullptr, PRECEDENCE_NONE},
	    /* FLOAT                */ {&Parser::ParseConstantExpression, nullptr, PRECEDENCE_NONE},
	    /* DOUBLE               */ {&Parser::ParseConstantExpression, nullptr, PRECEDENCE_NONE},
	    /* TRUE                 */ {&Parser::ParseConstantExpression, nullptr, PRECEDENCE_NONE},
	    /* FALSE                */ {&Parser::ParseConstantExpression, nullptr, PRECEDENCE_NONE},
	    /* CHARACTER            */ {&Parser::ParseConstantExpression, nullptr, PRECEDENCE_NONE},
	    /* STRING               */ {&Parser::ParseConstantExpression, nullptr, PRECEDENCE_NONE},

	    /* EOF                  */ {nullptr, nullptr, PRECEDENCE_NONE},
	};

} // namespace Wandelt
