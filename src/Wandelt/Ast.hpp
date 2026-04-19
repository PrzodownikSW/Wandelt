#pragma once

#include "Wandelt/Defines.hpp"
#include "Wandelt/String.hpp"
#include "Wandelt/Token.hpp"
#include "Wandelt/Type.hpp"
#include "Wandelt/Vector.hpp"

namespace Wandelt
{

	enum ResolveStatus
	{
		RESOLVE_STATUS_INVALID = 0,

		RESOLVE_STATUS_UNRESOLVED,
		RESOLVE_STATUS_RESOLVING,
		RESOLVE_STATUS_RESOLVED,

		RESOLVE_STATUS_COUNT,
	};

	const char* ResolveStatusToCStr(ResolveStatus status);

	// ---------------------------------------------------------------------------
	// Expression stuff
	// ---------------------------------------------------------------------------

	enum ExpressionType
	{
		EXPRESSION_TYPE_INVALID = 0,

		EXPRESSION_TYPE_CONSTANT,
		EXPRESSION_TYPE_IDENTIFIER,
		EXPRESSION_TYPE_CALL,

		EXPRESSION_TYPE_COUNT,
	};

	const char* ExpressionTypeToCStr(ExpressionType type);

	enum ConstantKind
	{
		CONSTANT_KIND_INVALID = 0,

		CONSTANT_KIND_INTEGER,
		CONSTANT_KIND_FLOAT,
		CONSTANT_KIND_DOUBLE,
		CONSTANT_KIND_BOOLEAN,

		CONSTANT_KIND_COUNT,
	};

	const char* ConstantKindToCStr(ConstantKind kind);

	struct ConstantExpression
	{
		ConstantKind kind;

		union { // TODO(PrzodownikSW): in future use big ints
			u64 integerValue;
			f32 floatValue;
			f64 doubleValue;
			bool booleanValue;
		};
	};

	struct IdentifierExpression
	{
		StringView name;
		struct Declaration* declarationRef;
	};

	struct CallExpression
	{
		StringView functionName;
		struct Declaration* declarationRef;
		Vector<struct Expression*> arguments;
	};

	struct Expression
	{
		ExpressionType type;
		Span span;

		ResolveStatus resolveStatus;
		Type* resolvedType;

		union {
			ConstantExpression constant;
			IdentifierExpression identifier;
			CallExpression call;
		};
	};

	// ---------------------------------------------------------------------------
	// Declaration stuff
	// ---------------------------------------------------------------------------

	enum DeclarationType
	{
		DECLARATION_TYPE_INVALID = 0,

		DECLARATION_TYPE_PACKAGE,
		DECLARATION_TYPE_VARIABLE,
		DECLARATION_TYPE_FUNCTION,

		DECLARATION_TYPE_COUNT,
	};

	const char* DeclarationTypeToCStr(DeclarationType type);

	struct PackageDeclaration
	{
		StringView name;
		bool isEntrypoint;
	};

	struct VariableDeclaration
	{
		StringView name;
		Type* type;
		Expression* initializer;
	};

	struct FunctionDeclaration
	{
		StringView name;
		Type* returnType;
		Vector<VariableDeclaration*> parameters;
		struct Statement* body;
	};

	struct Declaration
	{
		DeclarationType type;
		Span span;

		union {
			PackageDeclaration package;
			VariableDeclaration variable;
			FunctionDeclaration function;
		};
	};

	// ---------------------------------------------------------------------------
	// Statement stuff
	// ---------------------------------------------------------------------------

	enum StatementType
	{
		STATEMENT_TYPE_INVALID = 0,

		STATEMENT_TYPE_DECLARATION,
		STATEMENT_TYPE_EXPRESSION,
		STATEMENT_TYPE_RETURN,
		STATEMENT_TYPE_BLOCK,

		STATEMENT_TYPE_COUNT,
	};

	const char* StatementTypeToCStr(StatementType type);

	struct DeclarationStatement
	{
		Declaration* declaration;
	};

	struct ExpressionStatement
	{
		Expression* expression;
	};

	struct ReturnStatement
	{
		Expression* expression;
	};

	struct BlockStatement
	{
		Vector<struct Statement*> statements;
	};

	struct Statement
	{
		StatementType type;
		Span span;

		union {
			DeclarationStatement declaration;
			ExpressionStatement expression;
			ReturnStatement returnStmt;
			BlockStatement block;
		};
	};

} // namespace Wandelt
