#pragma once

#include "TestingFramework.hpp"

#include "Wandelt/Parser.hpp"

namespace Wandelt
{

	struct ExpectedDiagnostic
	{
		Diagnostics::Severity severity;
		u32 line;
		u32 col;
		const char* messageSubstring;
	};

	inline bool TestStringEquals(StringView actual, const char* expected)
	{
		u64 expectedLength = strlen(expected);
		return actual.Length() == expectedLength && memcmp(actual.Data(), expected, expectedLength) == 0;
	}

	inline Statement* GetTopLevelStatement(TranslationUnit* translationUnit, u64 index)
	{
		ASSERT(index < translationUnit->statements.Length(), "top-level statement index out of bounds");
		return translationUnit->statements[index];
	}

	inline Declaration* GetDeclarationFromStatement(Statement* statement)
	{
		ASSERT(statement->type == STATEMENT_TYPE_DECLARATION, "statement is not a declaration");
		return statement->declaration.declaration;
	}

	inline Declaration* GetDeclarationFromTopLevelStatement(TranslationUnit* translationUnit, u64 index)
	{
		return GetDeclarationFromStatement(GetTopLevelStatement(translationUnit, index));
	}

	inline Statement* GetFunctionBodyStatement(Declaration* declaration, u64 index)
	{
		ASSERT(declaration != nullptr, "declaration is null");
		ASSERT(declaration->type == DECLARATION_TYPE_FUNCTION, "declaration is not a function");
		ASSERT(declaration->function.body != nullptr, "function body is null");
		ASSERT(declaration->function.body->type == STATEMENT_TYPE_BLOCK, "function body is not a block");
		ASSERT(index < declaration->function.body->block.statements.Length(), "function body statement index out of bounds");

		return declaration->function.body->block.statements[index];
	}

	inline bool AssertBuiltinType(Type* type, BuiltinTypeKind expectedKind)
	{
		if (type == nullptr)
		{
			WDT_RECORD_FAILURE("  expected builtin type, got null\n");
			return false;
		}

		if (type->kind != TYPE_KIND_BUILTIN)
		{
			WDT_RECORD_FAILURE("  expected builtin type kind %d, got %d\n", TYPE_KIND_BUILTIN, type->kind);
			return false;
		}

		if (type->basic.kind != expectedKind)
		{
			WDT_RECORD_FAILURE("  expected builtin type %d, got %d\n", expectedKind, type->basic.kind);
			return false;
		}

		return true;
	}

	inline bool AssertArrayType(Type* type, u64 expectedLength, Type** outElementType = nullptr)
	{
		if (type == nullptr)
		{
			WDT_RECORD_FAILURE("  expected array type, got null\n");
			return false;
		}

		if (type->kind != TYPE_KIND_ARRAY)
		{
			WDT_RECORD_FAILURE("  expected array type kind %d, got %d\n", TYPE_KIND_ARRAY, type->kind);
			return false;
		}

		if (type->array.length != expectedLength)
		{
			WDT_RECORD_FAILURE("  expected array length %llu, got %llu\n", expectedLength, type->array.length);
			return false;
		}

		if (outElementType != nullptr)
			*outElementType = type->array.elementType;

		return true;
	}

	inline bool AssertPointerType(Type* type, Type** outPointeeType = nullptr)
	{
		if (type == nullptr)
		{
			WDT_RECORD_FAILURE("  expected pointer type, got null\n");
			return false;
		}

		if (type->kind != TYPE_KIND_POINTER)
		{
			WDT_RECORD_FAILURE("  expected pointer type kind %d, got %d\n", TYPE_KIND_POINTER, type->kind);
			return false;
		}

		if (outPointeeType != nullptr)
			*outPointeeType = type->pointer.pointeeType;

		return true;
	}

	inline bool AssertSliceType(Type* type, Type** outElementType = nullptr)
	{
		if (type == nullptr)
		{
			WDT_RECORD_FAILURE("  expected slice type, got null\n");
			return false;
		}

		if (type->kind != TYPE_KIND_SLICE)
		{
			WDT_RECORD_FAILURE("  expected slice type kind %d, got %d\n", TYPE_KIND_SLICE, type->kind);
			return false;
		}

		if (outElementType != nullptr)
			*outElementType = type->slice.elementType;

		return true;
	}

	inline bool AssertIdentifierExpression(Expression* expression, const char* expectedIdentifier)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected identifier expression, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_IDENTIFIER)
		{
			WDT_RECORD_FAILURE("  expected identifier expression type %d, got %d\n", EXPRESSION_TYPE_IDENTIFIER, expression->type);
			return false;
		}

		if (!TestStringEquals(expression->identifier.name, expectedIdentifier))
		{
			WDT_RECORD_FAILURE("  expected identifier \"%s\", got \"%.*s\"\n", expectedIdentifier, (int)expression->identifier.name.Length(),
			                   expression->identifier.name.Data());
			return false;
		}

		return true;
	}

	inline bool AssertIntegerConstant(Expression* expression, u64 expectedInteger)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected integer constant, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_CONSTANT)
		{
			WDT_RECORD_FAILURE("  expected constant expression type %d, got %d\n", EXPRESSION_TYPE_CONSTANT, expression->type);
			return false;
		}

		if (expression->constant.kind != CONSTANT_KIND_INTEGER)
		{
			WDT_RECORD_FAILURE("  expected integer constant kind %d, got %d\n", CONSTANT_KIND_INTEGER, expression->constant.kind);
			return false;
		}

		if (expression->constant.integerValue != expectedInteger)
		{
			WDT_RECORD_FAILURE("  expected integer constant %llu, got %llu\n", expectedInteger, expression->constant.integerValue);
			return false;
		}

		return true;
	}

	inline bool AssertBooleanConstant(Expression* expression, bool expectedBoolean)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected boolean constant, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_CONSTANT)
		{
			WDT_RECORD_FAILURE("  expected constant expression type %d, got %d\n", EXPRESSION_TYPE_CONSTANT, expression->type);
			return false;
		}

		if (expression->constant.kind != CONSTANT_KIND_BOOLEAN)
		{
			WDT_RECORD_FAILURE("  expected boolean constant kind %d, got %d\n", CONSTANT_KIND_BOOLEAN, expression->constant.kind);
			return false;
		}

		if (expression->constant.booleanValue != expectedBoolean)
		{
			WDT_RECORD_FAILURE("  expected boolean constant %d, got %d\n", expectedBoolean, expression->constant.booleanValue);
			return false;
		}

		return true;
	}

	inline bool AssertFloatConstant(Expression* expression, f32 expectedFloat)
	{
		const f32 tolerance = 0.0001f;

		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected float constant, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_CONSTANT)
		{
			WDT_RECORD_FAILURE("  expected constant expression type %d, got %d\n", EXPRESSION_TYPE_CONSTANT, expression->type);
			return false;
		}

		if (expression->constant.kind != CONSTANT_KIND_FLOAT)
		{
			WDT_RECORD_FAILURE("  expected float constant kind %d, got %d\n", CONSTANT_KIND_FLOAT, expression->constant.kind);
			return false;
		}

		if (expression->constant.floatValue < expectedFloat - tolerance || expression->constant.floatValue > expectedFloat + tolerance)
		{
			WDT_RECORD_FAILURE("  expected float constant %.6f, got %.6f\n", expectedFloat, expression->constant.floatValue);
			return false;
		}

		return true;
	}

	inline bool AssertDoubleConstant(Expression* expression, f64 expectedDouble)
	{
		const f64 tolerance = 0.0000001;

		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected double constant, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_CONSTANT)
		{
			WDT_RECORD_FAILURE("  expected constant expression type %d, got %d\n", EXPRESSION_TYPE_CONSTANT, expression->type);
			return false;
		}

		if (expression->constant.kind != CONSTANT_KIND_DOUBLE)
		{
			WDT_RECORD_FAILURE("  expected double constant kind %d, got %d\n", CONSTANT_KIND_DOUBLE, expression->constant.kind);
			return false;
		}

		if (expression->constant.doubleValue < expectedDouble - tolerance || expression->constant.doubleValue > expectedDouble + tolerance)
		{
			WDT_RECORD_FAILURE("  expected double constant %.12f, got %.12f\n", expectedDouble, expression->constant.doubleValue);
			return false;
		}

		return true;
	}

	inline bool AssertCharConstant(Expression* expression, char expectedChar)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected char constant, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_CONSTANT)
		{
			WDT_RECORD_FAILURE("  expected constant expression type %d, got %d\n", EXPRESSION_TYPE_CONSTANT, expression->type);
			return false;
		}

		if (expression->constant.kind != CONSTANT_KIND_CHAR)
		{
			WDT_RECORD_FAILURE("  expected char constant kind %d, got %d\n", CONSTANT_KIND_CHAR, expression->constant.kind);
			return false;
		}

		if (expression->constant.charValue != expectedChar)
		{
			WDT_RECORD_FAILURE("  expected char constant %d, got %d\n", (int)expectedChar, (int)expression->constant.charValue);
			return false;
		}

		return true;
	}

	inline bool AssertStringConstant(Expression* expression, const char* expectedString)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected string constant, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_CONSTANT)
		{
			WDT_RECORD_FAILURE("  expected constant expression type %d, got %d\n", EXPRESSION_TYPE_CONSTANT, expression->type);
			return false;
		}

		if (expression->constant.kind != CONSTANT_KIND_STRING)
		{
			WDT_RECORD_FAILURE("  expected string constant kind %d, got %d\n", CONSTANT_KIND_STRING, expression->constant.kind);
			return false;
		}

		if (!TestStringEquals(expression->constant.stringValue, expectedString))
		{
			WDT_RECORD_FAILURE("  expected string constant \"%s\", got \"%.*s\"\n", expectedString, (int)expression->constant.stringValue.Length(),
			                   expression->constant.stringValue.Data());
			return false;
		}

		return true;
	}

	inline bool AssertNullConstant(Expression* expression)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected null constant, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_CONSTANT)
		{
			WDT_RECORD_FAILURE("  expected constant expression type %d, got %d\n", EXPRESSION_TYPE_CONSTANT, expression->type);
			return false;
		}

		if (expression->constant.kind != CONSTANT_KIND_NULL)
		{
			WDT_RECORD_FAILURE("  expected null constant kind %d, got %d\n", CONSTANT_KIND_NULL, expression->constant.kind);
			return false;
		}

		return true;
	}

	inline bool AssertCastExpression(Expression* expression, BuiltinTypeKind expectedTargetKind)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected cast expression, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_CAST)
		{
			WDT_RECORD_FAILURE("  expected cast expression type %d, got %d\n", EXPRESSION_TYPE_CAST, expression->type);
			return false;
		}

		if (!AssertBuiltinType(expression->cast.targetType, expectedTargetKind))
			return false;

		if (expression->cast.expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected cast operand expression, got null\n");
			return false;
		}

		return true;
	}

	inline bool AssertCastExpression(Expression* expression, Type** outTargetType)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected cast expression, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_CAST)
		{
			WDT_RECORD_FAILURE("  expected cast expression type %d, got %d\n", EXPRESSION_TYPE_CAST, expression->type);
			return false;
		}

		if (expression->cast.targetType == nullptr)
		{
			WDT_RECORD_FAILURE("  expected cast target type, got null\n");
			return false;
		}

		if (expression->cast.expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected cast operand expression, got null\n");
			return false;
		}

		if (outTargetType != nullptr)
			*outTargetType = expression->cast.targetType;

		return true;
	}

	inline bool AssertCallExpression(Expression* expression, const char* expectedFunctionName, u64 expectedArgumentCount = 0u)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected call expression, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_CALL)
		{
			WDT_RECORD_FAILURE("  expected call expression type %d, got %d\n", EXPRESSION_TYPE_CALL, expression->type);
			return false;
		}

		if (!TestStringEquals(expression->call.functionName, expectedFunctionName))
		{
			WDT_RECORD_FAILURE("  expected call target \"%s\", got \"%.*s\"\n", expectedFunctionName, (int)expression->call.functionName.Length(),
			                   expression->call.functionName.Data());
			return false;
		}

		if (expression->call.arguments.Length() != expectedArgumentCount)
		{
			WDT_RECORD_FAILURE("  expected call with %llu arguments, got %llu\n", expectedArgumentCount, expression->call.arguments.Length());
			return false;
		}

		return true;
	}

	inline bool AssertUnaryExpression(Expression* expression, UnaryOperator expectedOperator, bool expectedPostfix = false)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected unary expression, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_UNARY)
		{
			WDT_RECORD_FAILURE("  expected unary expression type %d, got %d\n", EXPRESSION_TYPE_UNARY, expression->type);
			return false;
		}

		if (expression->unary.op != expectedOperator)
		{
			WDT_RECORD_FAILURE("  expected unary operator %d, got %d\n", expectedOperator, expression->unary.op);
			return false;
		}

		if (expression->unary.isPostfix != expectedPostfix)
		{
			WDT_RECORD_FAILURE("  expected unary isPostfix=%d, got %d\n", expectedPostfix, expression->unary.isPostfix);
			return false;
		}

		if (expression->unary.operand == nullptr)
		{
			WDT_RECORD_FAILURE("  expected unary operand, got null\n");
			return false;
		}

		return true;
	}

	inline bool AssertBinaryExpression(Expression* expression, BinaryOperator expectedOperator)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected binary expression, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_BINARY)
		{
			WDT_RECORD_FAILURE("  expected binary expression type %d, got %d\n", EXPRESSION_TYPE_BINARY, expression->type);
			return false;
		}

		if (expression->binary.op != expectedOperator)
		{
			WDT_RECORD_FAILURE("  expected binary operator %d, got %d\n", expectedOperator, expression->binary.op);
			return false;
		}

		if (expression->binary.left == nullptr || expression->binary.right == nullptr)
		{
			WDT_RECORD_FAILURE("  expected binary operands, got null\n");
			return false;
		}

		return true;
	}

	inline bool AssertGroupExpression(Expression* expression)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected group expression, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_GROUP)
		{
			WDT_RECORD_FAILURE("  expected group expression type %d, got %d\n", EXPRESSION_TYPE_GROUP, expression->type);
			return false;
		}

		if (expression->group.inner == nullptr)
		{
			WDT_RECORD_FAILURE("  expected grouped inner expression, got null\n");
			return false;
		}

		return true;
	}

	inline bool AssertIncDecExpression(Expression* expression, bool expectedIncrement, bool expectedPostfix)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected inc/dec expression, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_INCDEC)
		{
			WDT_RECORD_FAILURE("  expected inc/dec expression type %d, got %d\n", EXPRESSION_TYPE_INCDEC, expression->type);
			return false;
		}

		if (expression->incdec.isIncrement != expectedIncrement)
		{
			WDT_RECORD_FAILURE("  expected isIncrement=%d, got %d\n", expectedIncrement, expression->incdec.isIncrement);
			return false;
		}

		if (expression->incdec.isPostfix != expectedPostfix)
		{
			WDT_RECORD_FAILURE("  expected isPostfix=%d, got %d\n", expectedPostfix, expression->incdec.isPostfix);
			return false;
		}

		if (expression->incdec.operand == nullptr)
		{
			WDT_RECORD_FAILURE("  expected inc/dec operand, got null\n");
			return false;
		}

		return true;
	}

	inline bool AssertAssignmentExpression(Expression* expression, AssignmentOperator expectedOperator)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected assignment expression, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_ASSIGNMENT)
		{
			WDT_RECORD_FAILURE("  expected assignment expression type %d, got %d\n", EXPRESSION_TYPE_ASSIGNMENT, expression->type);
			return false;
		}

		if (expression->assignment.op != expectedOperator)
		{
			WDT_RECORD_FAILURE("  expected assignment operator %d, got %d\n", expectedOperator, expression->assignment.op);
			return false;
		}

		if (expression->assignment.left == nullptr || expression->assignment.right == nullptr)
		{
			WDT_RECORD_FAILURE("  expected assignment operands, got null\n");
			return false;
		}

		return true;
	}

	inline bool AssertArrayLiteralExpression(Expression* expression, u64 expectedItemCount, bool expectedRepeatLastElement = false)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected array literal expression, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_ARRAY_LITERAL)
		{
			WDT_RECORD_FAILURE("  expected array literal expression type %d, got %d\n", EXPRESSION_TYPE_ARRAY_LITERAL, expression->type);
			return false;
		}

		if (expression->arrayLiteral.items.Length() != expectedItemCount)
		{
			WDT_RECORD_FAILURE("  expected array literal with %llu items, got %llu\n", expectedItemCount, expression->arrayLiteral.items.Length());
			return false;
		}

		if (expression->arrayLiteral.repeatLastElement != expectedRepeatLastElement)
		{
			WDT_RECORD_FAILURE("  expected repeatLastElement=%d, got %d\n", expectedRepeatLastElement, expression->arrayLiteral.repeatLastElement);
			return false;
		}

		return true;
	}

	inline bool AssertIndexExpression(Expression* expression, Expression** outTarget = nullptr, Expression** outIndex = nullptr)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected index expression, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_INDEX)
		{
			WDT_RECORD_FAILURE("  expected index expression type %d, got %d\n", EXPRESSION_TYPE_INDEX, expression->type);
			return false;
		}

		if (expression->index.target == nullptr || expression->index.index == nullptr)
		{
			WDT_RECORD_FAILURE("  expected index target and index expressions, got null\n");
			return false;
		}

		if (outTarget != nullptr)
			*outTarget = expression->index.target;

		if (outIndex != nullptr)
			*outIndex = expression->index.index;

		return true;
	}

	inline bool AssertIntrinsicExpression(Expression* expression, IntrinsicKind expectedKind, u64 expectedArgumentCount = 0u)
	{
		if (expression == nullptr)
		{
			WDT_RECORD_FAILURE("  expected intrinsic expression, got null\n");
			return false;
		}

		if (expression->type != EXPRESSION_TYPE_INTRINSIC)
		{
			WDT_RECORD_FAILURE("  expected intrinsic expression type %d, got %d\n", EXPRESSION_TYPE_INTRINSIC, expression->type);
			return false;
		}

		if (expression->intrinsic.kind != expectedKind)
		{
			WDT_RECORD_FAILURE("  expected intrinsic kind %d, got %d\n", expectedKind, expression->intrinsic.kind);
			return false;
		}

		if (expression->intrinsic.arguments.Length() != expectedArgumentCount)
		{
			WDT_RECORD_FAILURE("  expected intrinsic with %llu arguments, got %llu\n", expectedArgumentCount,
			                   expression->intrinsic.arguments.Length());
			return false;
		}

		return true;
	}

	inline bool AssertFunctionParameter(Declaration* parameter, BuiltinTypeKind expectedType, const char* expectedName)
	{
		if (parameter == nullptr)
		{
			WDT_RECORD_FAILURE("  expected function parameter, got null\n");
			return false;
		}

		if (parameter->type != DECLARATION_TYPE_VARIABLE)
		{
			WDT_RECORD_FAILURE("  expected parameter declaration type %d, got %d\n", DECLARATION_TYPE_VARIABLE, parameter->type);
			return false;
		}

		if (!AssertBuiltinType(parameter->variable.type, expectedType))
			return false;

		if (!TestStringEquals(parameter->variable.name, expectedName))
		{
			WDT_RECORD_FAILURE("  expected parameter name \"%s\", got \"%.*s\"\n", expectedName, (int)parameter->variable.name.Length(),
			                   parameter->variable.name.Data());
			return false;
		}

		return true;
	}

	inline bool AssertPositionalCallArgument(Expression* expression, u64 index, Expression** outArgumentExpression)
	{
		if (expression == nullptr || expression->type != EXPRESSION_TYPE_CALL)
		{
			WDT_RECORD_FAILURE("  expected call expression when checking argument %llu\n", index);
			return false;
		}

		if (index >= expression->call.arguments.Length())
		{
			WDT_RECORD_FAILURE("  argument index %llu out of bounds (length %llu)\n", index, expression->call.arguments.Length());
			return false;
		}

		if (expression->call.arguments[index].name)
		{
			WDT_RECORD_FAILURE("  expected positional argument at index %llu, got named argument \"%.*s\"\n", index,
			                   (int)expression->call.arguments[index].name.Length(), expression->call.arguments[index].name.Data());
			return false;
		}

		*outArgumentExpression = expression->call.arguments[index].expression;
		return true;
	}

	inline bool AssertNamedCallArgument(Expression* expression, u64 index, const char* expectedName, Expression** outArgumentExpression)
	{
		if (expression == nullptr || expression->type != EXPRESSION_TYPE_CALL)
		{
			WDT_RECORD_FAILURE("  expected call expression when checking argument %llu\n", index);
			return false;
		}

		if (index >= expression->call.arguments.Length())
		{
			WDT_RECORD_FAILURE("  argument index %llu out of bounds (length %llu)\n", index, expression->call.arguments.Length());
			return false;
		}

		const CallExpression::Argument& argument = expression->call.arguments[index];
		if (!TestStringEquals(argument.name, expectedName))
		{
			WDT_RECORD_FAILURE("  expected named argument \"%s\", got \"%.*s\"\n", expectedName, (int)argument.name.Length(), argument.name.Data());
			return false;
		}

		*outArgumentExpression = argument.expression;
		return true;
	}

	inline bool AssertCapturedDiagnostic(Diagnostics* diagnostics, u32 index, const ExpectedDiagnostic& expected)
	{
		if (diagnostics == nullptr)
		{
			WDT_RECORD_FAILURE("  expected diagnostics object, got null\n");
			return false;
		}

		if (index >= diagnostics->CapturedCount())
		{
			WDT_RECORD_FAILURE("  expected captured diagnostic at index %u, but count is %u\n", index, diagnostics->CapturedCount());
			return false;
		}

		Diagnostics::Entry* entry = diagnostics->GetCaptured(index);
		if (entry->severity != expected.severity)
		{
			WDT_RECORD_FAILURE("  expected diagnostic severity %d, got %d\n", expected.severity, entry->severity);
			return false;
		}

		if (entry->line != expected.line)
		{
			WDT_RECORD_FAILURE("  expected diagnostic line %u, got %u\n", expected.line, entry->line);
			return false;
		}

		if (entry->col != expected.col)
		{
			WDT_RECORD_FAILURE("  expected diagnostic column %u, got %u\n", expected.col, entry->col);
			return false;
		}

		if (strstr(entry->message, expected.messageSubstring) == nullptr)
		{
			WDT_RECORD_FAILURE("  expected diagnostic containing \"%s\", got \"%s\"\n", expected.messageSubstring, entry->message);
			return false;
		}

		return true;
	}

} // namespace Wandelt
