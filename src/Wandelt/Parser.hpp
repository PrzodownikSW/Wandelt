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

		PRECEDENCE_ASSIGNMENT,     // =, +=, -=, *=, /=, %=
		PRECEDENCE_EQUALITY,       // ==, !=
		PRECEDENCE_COMPARISON,     // <, >, <=, >=
		PRECEDENCE_ADDITIVE,       // +, -
		PRECEDENCE_MULTIPLICATIVE, // *, /, %
		PRECEDENCE_PREFIX,         // -x, !x, ++x, --x
		PRECEDENCE_POSTFIX,        // call(), x++, x--
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
		enum class ParseScope
		{
			TopLevel,
			Block
		};

		Statement* ParseTopLevelStatement();
		Statement* ParseInnerStatement();
		Statement* ParseDeclarationStatement();
		Statement* ParseExpressionStatement();
		Statement* ParseReturnStatement();
		Statement* ParseBlockStatement();
		Statement* ParseIfStatement();
		Statement* ParseWhileStatement();
		Statement* ParseForStatement();
		Statement* ParseBreakStatement();
		Statement* ParseContinueStatement();

		Declaration* ParseDeclaration();
		Declaration* ParsePackageDeclaration();
		Declaration* ParseVariableDeclaration();
		Declaration* ParseFunctionDeclaration();
		bool ParseFunctionParameter(Declaration** outParameter);

		Expression* ParseExpression();
		Expression* ParseExpressionWithPrecedence(Precedence minPrecedence);
		Expression* ParseConstantExpression();
		Expression* ParseUnaryExpression();
		Expression* ParseBinaryExpression(Expression* left);
		Expression* ParseGroupExpression();
		Expression* ParseArrayLiteralExpression();
		Expression* ParseIdentifierExpression();
		Expression* ParseIntrinsicExpression();
		Expression* ParseCastExpression();
		Expression* ParsePrefixIncDecExpression();
		Expression* ParsePostfixIncDecExpression(Expression* left);
		Expression* ParseDerefExpression(Expression* left);
		Expression* ParseCallExpression(Expression* left);
		Expression* ParseIndexExpression(Expression* left);
		bool ParseCallArgument(CallExpression::Argument* outArgument, bool* outIsNamed);
		Expression* ParseAssignmentExpression(Expression* left);

		void RecoverFromError(ParseScope scope);

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
