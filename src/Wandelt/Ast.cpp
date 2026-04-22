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

		case EXPRESSION_TYPE_IDENTIFIER:
			return "IdentifierExpression";

		case EXPRESSION_TYPE_CALL:
			return "CallExpression";

		case EXPRESSION_TYPE_CAST:
			return "CastExpression";

		case EXPRESSION_TYPE_COUNT:
			ASSERT(false, "Invalid expression type!");
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

		case CONSTANT_KIND_COUNT:
			ASSERT(false, "Invalid constant kind!");
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

		case STATEMENT_TYPE_COUNT:
			ASSERT(false, "Invalid statement type!");
		}

		UNREACHABLE();
	}

} // namespace Wandelt
