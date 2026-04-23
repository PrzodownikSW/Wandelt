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
		RESOLVE_STATUS_UNRESOLVED,
		RESOLVE_STATUS_RESOLVING,
		RESOLVE_STATUS_RESOLVED,
	};

	const char* ResolveStatusToCStr(ResolveStatus status);

	// ---------------------------------------------------------------------------
	// Expression stuff
	// ---------------------------------------------------------------------------

	enum ExpressionType
	{
		EXPRESSION_TYPE_INVALID = 0,

		EXPRESSION_TYPE_CONSTANT,
		EXPRESSION_TYPE_UNARY,
		EXPRESSION_TYPE_BINARY,
		EXPRESSION_TYPE_GROUP,
		EXPRESSION_TYPE_IDENTIFIER,
		EXPRESSION_TYPE_CAST,
		EXPRESSION_TYPE_INCDEC,
		EXPRESSION_TYPE_CALL,
		EXPRESSION_TYPE_ASSIGNMENT,

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
		CONSTANT_KIND_CHAR,
		CONSTANT_KIND_STRING,

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
			char charValue;
			StringView stringValue;
		};
	};

	enum UnaryOperator
	{
		UNARY_OPERATOR_INVALID = 0,

		UNARY_OPERATOR_NEGATE, // -x

		UNARY_OPERATOR_COUNT,
	};

	const char* UnaryOperatorToCStr(UnaryOperator op);
	const char* UnaryOperatorToTokenCStr(UnaryOperator op);

	struct UnaryExpression
	{
		UnaryOperator op;
		struct Expression* operand;
	};

	enum BinaryOperator
	{
		BINARY_OPERATOR_INVALID = 0,

		BINARY_OPERATOR_ADD, // +
		BINARY_OPERATOR_SUB, // -
		BINARY_OPERATOR_MUL, // *
		BINARY_OPERATOR_DIV, // /
		BINARY_OPERATOR_EQ,  // ==
		BINARY_OPERATOR_NEQ, // !=
		BINARY_OPERATOR_LT,  // <
		BINARY_OPERATOR_GT,  // >
		BINARY_OPERATOR_LEQ, // <=
		BINARY_OPERATOR_GEQ, // >=

		BINARY_OPERATOR_COUNT,
	};

	const char* BinaryOperatorToCStr(BinaryOperator op);
	const char* BinaryOperatorToTokenCStr(BinaryOperator op);
	BinaryOperator TokenTypeToBinaryOperator(TokenType tokenType);
	bool IsBinaryOpAComparison(BinaryOperator op);
	bool IsBinaryOpEqualityComparison(BinaryOperator op);
	bool IsBinaryOpRelationalComparison(BinaryOperator op);

	struct BinaryExpression
	{
		BinaryOperator op;
		struct Expression* left;
		struct Expression* right;
	};

	struct GroupExpression
	{
		struct Expression* inner;
	};

	struct IdentifierExpression
	{
		StringView name;
		struct Declaration* declarationRef;
	};

	struct CastExpression
	{
		Type* targetType;
		struct Expression* expression;
	};

	struct IncDecExpression
	{
		struct Expression* operand;
		bool isIncrement;
		bool isPostfix;
	};

	struct CallExpression
	{
		struct Argument
		{
			StringView name;
			Span span;
			struct Expression* expression;
		};

		StringView functionName;
		struct Declaration* declarationRef;
		Vector<Argument> arguments;
	};

	enum AssignmentOperator
	{
		ASSIGNMENT_OPERATOR_INVALID = 0,

		ASSIGNMENT_OPERATOR_PURE, // =
		ASSIGNMENT_OPERATOR_ADD,  // +=
		ASSIGNMENT_OPERATOR_SUB,  // -=
		ASSIGNMENT_OPERATOR_MUL,  // *=
		ASSIGNMENT_OPERATOR_DIV,  // /=

		ASSIGNMENT_OPERATOR_COUNT,
	};

	const char* AssignmentOperatorToCStr(AssignmentOperator op);
	const char* AssignmentOperatorToTokenCStr(AssignmentOperator op);
	AssignmentOperator TokenTypeToAssignmentOperator(TokenType tokenType);
	BinaryOperator AssignmentOperatorToBinaryOperator(AssignmentOperator op);

	struct AssignmentExpression
	{
		AssignmentOperator op;
		struct Expression* left;
		struct Expression* right;
	};

	struct Expression
	{
		ExpressionType type;
		Span span;

		ResolveStatus resolveStatus;
		Type* resolvedType;

		union {
			ConstantExpression constant;
			UnaryExpression unary;
			BinaryExpression binary;
			GroupExpression group;
			IdentifierExpression identifier;
			CastExpression cast;
			IncDecExpression incdec;
			CallExpression call;
			AssignmentExpression assignment;
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
		Vector<struct Declaration*> parameters;
		struct Statement* body;
	};

	struct Declaration
	{
		DeclarationType type;
		Span span;
		ResolveStatus resolveStatus;

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
		STATEMENT_TYPE_IF,
		STATEMENT_TYPE_WHILE,
		STATEMENT_TYPE_FOR,
		STATEMENT_TYPE_BREAK,
		STATEMENT_TYPE_CONTINUE,

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
		bool discarded;
	};

	struct ReturnStatement
	{
		Expression* expression;
	};

	struct BlockStatement
	{
		Vector<struct Statement*> statements;
	};

	struct IfStatement
	{
		struct Expression* condition;
		struct Statement* thenBlock;
		struct Statement* elseBranch; // another if or else block or nullptr
	};

	struct WhileStatement
	{
		struct Expression* condition;
		struct Statement* body;
	};

	struct ForStatement
	{
		struct Statement* init; // declaration statement or expression statement
		struct Expression* condition;
		struct Expression* increment;
		struct Statement* body;
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
			IfStatement ifStmt;
			WhileStatement whileStmt;
			ForStatement forStmt;
		};
	};

} // namespace Wandelt
