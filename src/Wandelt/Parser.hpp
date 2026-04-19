#pragma once

#include "Wandelt/Ast.hpp"
#include "Wandelt/Defines.hpp"
#include "Wandelt/Lexer.hpp"

namespace Wandelt
{

	struct TranslationUnit
	{
		const File* file;
		Vector<Statement*> statements;
	};

	enum Precedence
	{
		PRECEDENCE_NONE = 0,

		PRECEDENCE_POSTFIX, // call()
		PRECEDENCE_PRIMARY,

		PRECEDENCE_COUNT
	};

	class Parser;

	using PrefixParseFn = Expression* (Parser::*)();
	using InfixParseFn  = Expression* (Parser::*)(Expression * left);

	struct ParseRule
	{
		PrefixParseFn prefix; // null denotation, prefix
		InfixParseFn infix;   // left denotation, infix/postfix
		Precedence precedence;
	};

	class Parser
	{
	public:
		Parser(Allocator* stmtAllocator, Allocator* declAllocator, Allocator* exprAllocator, Lexer* lexer, Diagnostics* diagnostics);
		~Parser() = default;

		NONCOPYABLE(Parser);
		NONMOVABLE(Parser);

	public:
		TranslationUnit Parse();

	private:
		Statement* ParseTopLevelStatement();
		Statement* ParseInnerStatement();
		Statement* ParseDeclarationStatement();
		Statement* ParseExpressionStatement();
		Statement* ParseReturnStatement();
		Statement* ParseBlockStatement();

		Declaration* ParseDeclaration();
		Declaration* ParsePackageDeclaration();
		Declaration* ParseVariableDeclaration();
		Declaration* ParseFunctionDeclaration();

		Expression* ParseExpression();
		Expression* ParseExpressionWithPrecedence(Precedence minPrecedence);
		Expression* ParseConstantExpression();
		Expression* ParseIdentifierExpression();
		Expression* ParseCallExpression(Expression* left);

		void RecoverFromError();

		bool ParseToken(TokenType expectedType);
		bool ParseIdentifier(StringView* outIdentifier);
		bool ParseType(Type** outType);

	private:
		static const ParseRule s_ParseRules[TOKEN_TYPE_COUNT];

		Allocator* m_StmtAllocator = nullptr;
		Allocator* m_DeclAllocator = nullptr;
		Allocator* m_ExprAllocator = nullptr;

		Lexer* m_Lexer             = nullptr;
		Diagnostics* m_Diagnostics = nullptr;

		Token m_PreviousToken;

		TranslationUnit m_TranslationUnit;
	};

} // namespace Wandelt
