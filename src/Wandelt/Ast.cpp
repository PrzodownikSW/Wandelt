#include "Ast.hpp"

namespace Wandelt
{

	const char* ResolveStatusToCStr(ResolveStatus status)
	{
		switch (status)
		{
		case RESOLVE_STATUS_UNRESOLVED:
			return "Unresolved";

		case RESOLVE_STATUS_RESOLVING:
			return "Resolving";

		case RESOLVE_STATUS_RESOLVED:
			return "Resolved";
		}

		UNREACHABLE();
	}

	const char* ExpressionTypeToCStr(ExpressionType type)
	{
		switch (type)
		{
		case EXPRESSION_TYPE_INVALID:
			ASSERT(false, "Invalid expression type!");
			break;

		case EXPRESSION_TYPE_CONSTANT:
			return "ConstantExpression";

		case EXPRESSION_TYPE_UNARY:
			return "UnaryExpression";

		case EXPRESSION_TYPE_BINARY:
			return "BinaryExpression";

		case EXPRESSION_TYPE_GROUP:
			return "GroupExpression";

		case EXPRESSION_TYPE_INCDEC:
			return "IncDecExpression";

		case EXPRESSION_TYPE_IDENTIFIER:
			return "IdentifierExpression";

		case EXPRESSION_TYPE_CALL:
			return "CallExpression";

		case EXPRESSION_TYPE_CAST:
			return "CastExpression";

		case EXPRESSION_TYPE_ASSIGNMENT:
			return "AssignmentExpression";

		case EXPRESSION_TYPE_ARRAY_LITERAL:
			return "ArrayLiteralExpression";

		case EXPRESSION_TYPE_INDEX:
			return "IndexExpression";

		case EXPRESSION_TYPE_INTRINSIC:
			return "IntrinsicExpression";

		case EXPRESSION_TYPE_COUNT:
			ASSERT(false, "Invalid expression type!");
			break;
		}

		UNREACHABLE();
	}

	const char* IntrinsicKindToCStr(IntrinsicKind kind)
	{
		switch (kind)
		{
		case INTRINSIC_KIND_INVALID:
			ASSERT(false, "Invalid intrinsic kind!");
			break;

		case INTRINSIC_KIND_LEN:
			return "$len";

		case INTRINSIC_KIND_COUNT:
			ASSERT(false, "Invalid intrinsic kind!");
			break;
		}

		UNREACHABLE();
	}

	const char* ConstantKindToCStr(ConstantKind kind)
	{
		switch (kind)
		{
		case CONSTANT_KIND_INVALID:
			ASSERT(false, "Invalid constant kind!");
			break;

		case CONSTANT_KIND_INTEGER:
			return "IntegerConstant";

		case CONSTANT_KIND_FLOAT:
			return "FloatConstant";

		case CONSTANT_KIND_DOUBLE:
			return "DoubleConstant";

		case CONSTANT_KIND_BOOLEAN:
			return "BooleanConstant";

		case CONSTANT_KIND_CHAR:
			return "CharConstant";

		case CONSTANT_KIND_STRING:
			return "StringConstant";

		case CONSTANT_KIND_NULL:
			return "NullConstant";

		case CONSTANT_KIND_COUNT:
			ASSERT(false, "Invalid constant kind!");
		}

		UNREACHABLE();
	}

	const char* UnaryOperatorToCStr(UnaryOperator op)
	{
		switch (op)
		{
		case UNARY_OPERATOR_INVALID:
			ASSERT(false, "Invalid unary operator!");
			break;

		case UNARY_OPERATOR_NEGATE:
			return "Negate";

		case UNARY_OPERATOR_ADDRESS_OF:
			return "AddressOf";

		case UNARY_OPERATOR_DEREF:
			return "Dereference";

		case UNARY_OPERATOR_COUNT:
			ASSERT(false, "Invalid unary operator!");
		}

		UNREACHABLE();
	}

	const char* UnaryOperatorToTokenCStr(UnaryOperator op)
	{
		switch (op)
		{
		case UNARY_OPERATOR_INVALID:
			ASSERT(false, "Invalid unary operator!");
			break;

		case UNARY_OPERATOR_NEGATE:
			return "-";

		case UNARY_OPERATOR_ADDRESS_OF:
			return "&";

		case UNARY_OPERATOR_DEREF:
			return "^";

		case UNARY_OPERATOR_COUNT:
			ASSERT(false, "Invalid unary operator!");
		}

		UNREACHABLE();
	}

	const char* BinaryOperatorToCStr(BinaryOperator op)
	{
		switch (op)
		{
		case BINARY_OPERATOR_INVALID:
			ASSERT(false, "Invalid binary operator!");
			break;

		case BINARY_OPERATOR_ADD:
			return "Add";

		case BINARY_OPERATOR_SUB:
			return "Subtract";

		case BINARY_OPERATOR_MUL:
			return "Multiply";

		case BINARY_OPERATOR_DIV:
			return "Divide";

		case BINARY_OPERATOR_EQ:
			return "Equal";

		case BINARY_OPERATOR_NEQ:
			return "NotEqual";

		case BINARY_OPERATOR_LT:
			return "LessThan";

		case BINARY_OPERATOR_GT:
			return "GreaterThan";

		case BINARY_OPERATOR_LEQ:
			return "LessThanOrEqual";

		case BINARY_OPERATOR_GEQ:
			return "GreaterThanOrEqual";

		case BINARY_OPERATOR_COUNT:
			ASSERT(false, "Invalid binary operator!");
		}

		UNREACHABLE();
	}

	const char* BinaryOperatorToTokenCStr(BinaryOperator op)
	{
		switch (op)
		{
		case BINARY_OPERATOR_INVALID:
			ASSERT(false, "Invalid binary operator!");
			break;

		case BINARY_OPERATOR_ADD:
			return "+";

		case BINARY_OPERATOR_SUB:
			return "-";

		case BINARY_OPERATOR_MUL:
			return "*";

		case BINARY_OPERATOR_DIV:
			return "/";

		case BINARY_OPERATOR_EQ:
			return "==";

		case BINARY_OPERATOR_NEQ:
			return "!=";

		case BINARY_OPERATOR_LT:
			return "<";

		case BINARY_OPERATOR_GT:
			return ">";

		case BINARY_OPERATOR_LEQ:
			return "<=";

		case BINARY_OPERATOR_GEQ:
			return ">=";

		case BINARY_OPERATOR_COUNT:
			ASSERT(false, "Invalid binary operator!");
		}

		UNREACHABLE();
	}

	BinaryOperator TokenTypeToBinaryOperator(TokenType tokenType)
	{
		switch (tokenType)
		{
		case TOKEN_TYPE_PLUS:
			return BINARY_OPERATOR_ADD;

		case TOKEN_TYPE_MINUS:
			return BINARY_OPERATOR_SUB;

		case TOKEN_TYPE_STAR:
			return BINARY_OPERATOR_MUL;

		case TOKEN_TYPE_SLASH:
			return BINARY_OPERATOR_DIV;

		case TOKEN_TYPE_EQUAL_EQUAL:
			return BINARY_OPERATOR_EQ;

		case TOKEN_TYPE_BANG_EQUAL:
			return BINARY_OPERATOR_NEQ;

		case TOKEN_TYPE_LESS:
			return BINARY_OPERATOR_LT;

		case TOKEN_TYPE_GREATER:
			return BINARY_OPERATOR_GT;

		case TOKEN_TYPE_LESS_EQUAL:
			return BINARY_OPERATOR_LEQ;

		case TOKEN_TYPE_GREATER_EQUAL:
			return BINARY_OPERATOR_GEQ;

		default:
			ASSERT(false, "Token type is not a binary operator!");
			return BINARY_OPERATOR_INVALID;
		}
	}

	bool IsBinaryOpAComparison(BinaryOperator op)
	{
		return IsBinaryOpEqualityComparison(op) || IsBinaryOpRelationalComparison(op);
	}

	bool IsBinaryOpEqualityComparison(BinaryOperator op)
	{
		return op == BINARY_OPERATOR_EQ || op == BINARY_OPERATOR_NEQ;
	}

	bool IsBinaryOpRelationalComparison(BinaryOperator op)
	{
		return op == BINARY_OPERATOR_LT || op == BINARY_OPERATOR_GT || op == BINARY_OPERATOR_LEQ || op == BINARY_OPERATOR_GEQ;
	}

	const char* AssignmentOperatorToCStr(AssignmentOperator op)
	{
		switch (op)
		{
		case ASSIGNMENT_OPERATOR_INVALID:
			ASSERT(false, "Invalid assignment operator!");
			break;

		case ASSIGNMENT_OPERATOR_PURE:
			return "PureAssignment";

		case ASSIGNMENT_OPERATOR_ADD:
			return "AddAssignment";

		case ASSIGNMENT_OPERATOR_SUB:
			return "SubtractAssignment";

		case ASSIGNMENT_OPERATOR_MUL:
			return "MultiplyAssignment";

		case ASSIGNMENT_OPERATOR_DIV:
			return "DivideAssignment";

		case ASSIGNMENT_OPERATOR_COUNT:
			ASSERT(false, "Invalid assignment operator!");
		}

		UNREACHABLE();
	}

	const char* AssignmentOperatorToTokenCStr(AssignmentOperator op)
	{
		switch (op)
		{
		case ASSIGNMENT_OPERATOR_INVALID:
			ASSERT(false, "Invalid assignment operator!");
			break;

		case ASSIGNMENT_OPERATOR_PURE:
			return "=";

		case ASSIGNMENT_OPERATOR_ADD:
			return "+=";

		case ASSIGNMENT_OPERATOR_SUB:
			return "-=";

		case ASSIGNMENT_OPERATOR_MUL:
			return "*=";

		case ASSIGNMENT_OPERATOR_DIV:
			return "/=";

		case ASSIGNMENT_OPERATOR_COUNT:
			ASSERT(false, "Invalid assignment operator!");
		}

		UNREACHABLE();
	}

	AssignmentOperator TokenTypeToAssignmentOperator(TokenType tokenType)
	{
		switch (tokenType)
		{
		case TOKEN_TYPE_EQUALS:
			return ASSIGNMENT_OPERATOR_PURE;

		case TOKEN_TYPE_PLUS_EQUAL:
			return ASSIGNMENT_OPERATOR_ADD;

		case TOKEN_TYPE_MINUS_EQUAL:
			return ASSIGNMENT_OPERATOR_SUB;

		case TOKEN_TYPE_STAR_EQUAL:
			return ASSIGNMENT_OPERATOR_MUL;

		case TOKEN_TYPE_SLASH_EQUAL:
			return ASSIGNMENT_OPERATOR_DIV;

		default:
			ASSERT(false, "Token type is not an assignment operator!");
			return ASSIGNMENT_OPERATOR_INVALID;
		}
	}

	BinaryOperator AssignmentOperatorToBinaryOperator(AssignmentOperator op)
	{
		switch (op)
		{
		case ASSIGNMENT_OPERATOR_INVALID:
			ASSERT(false, "Invalid assignment operator!");
			break;

		case ASSIGNMENT_OPERATOR_PURE:
			return BINARY_OPERATOR_INVALID;

		case ASSIGNMENT_OPERATOR_ADD:
			return BINARY_OPERATOR_ADD;

		case ASSIGNMENT_OPERATOR_SUB:
			return BINARY_OPERATOR_SUB;

		case ASSIGNMENT_OPERATOR_MUL:
			return BINARY_OPERATOR_MUL;

		case ASSIGNMENT_OPERATOR_DIV:
			return BINARY_OPERATOR_DIV;

		case ASSIGNMENT_OPERATOR_COUNT:
			ASSERT(false, "Invalid assignment operator!");
		}

		UNREACHABLE();
	}

	const char* DeclarationTypeToCStr(DeclarationType type)
	{
		switch (type)
		{
		case DECLARATION_TYPE_INVALID:
			ASSERT(false, "Invalid declaration type!");
			break;

		case DECLARATION_TYPE_PACKAGE:
			return "PackageDeclaration";

		case DECLARATION_TYPE_VARIABLE:
			return "VariableDeclaration";

		case DECLARATION_TYPE_FUNCTION:
			return "FunctionDeclaration";

		case DECLARATION_TYPE_COUNT:
			ASSERT(false, "Invalid declaration type!");
		}

		UNREACHABLE();
	}

	const char* StatementTypeToCStr(StatementType type)
	{
		switch (type)
		{
		case STATEMENT_TYPE_INVALID:
			ASSERT(false, "Invalid statement type!");
			break;

		case STATEMENT_TYPE_DECLARATION:
			return "DeclarationStatement";

		case STATEMENT_TYPE_EXPRESSION:
			return "ExpressionStatement";

		case STATEMENT_TYPE_RETURN:
			return "ReturnStatement";

		case STATEMENT_TYPE_BLOCK:
			return "BlockStatement";

		case STATEMENT_TYPE_IF:
			return "IfStatement";

		case STATEMENT_TYPE_WHILE:
			return "WhileStatement";

		case STATEMENT_TYPE_FOR:
			return "ForStatement";

		case STATEMENT_TYPE_BREAK:
			return "BreakStatement";

		case STATEMENT_TYPE_CONTINUE:
			return "ContinueStatement";

		case STATEMENT_TYPE_COUNT:
			ASSERT(false, "Invalid statement type!");
		}

		UNREACHABLE();
	}

} // namespace Wandelt
