#include "ParserTests.hpp"

#include "Wandelt/Parser.hpp"

namespace Wandelt
{

	struct BuiltinTypeCase
	{
		const char* source;
		BuiltinTypeKind expectedKind;
	};

	struct ExpectedDiagnostic
	{
		Diagnostics::Severity severity;
		u32 line;
		u32 col;
		const char* messageSubstring;
	};

	static TranslationUnit ParseSource(Allocator* alloc, const char* source, Diagnostics* diagnostics)
	{
		File file = MakeTestFile(alloc, source);
		Lexer lexer{&file, diagnostics};
		Parser parser{alloc, alloc, alloc, &lexer, diagnostics};

		return parser.Parse();
	}

	static Statement* GetTopLevelStatement(TranslationUnit* translationUnit, u64 index)
	{
		ASSERT(index < translationUnit->statements.Length(), "top-level statement index out of bounds");
		return translationUnit->statements[index];
	}

	static Declaration* GetDeclarationFromStatement(Statement* statement)
	{
		ASSERT(statement->type == STATEMENT_TYPE_DECLARATION, "statement is not a declaration");
		return statement->declaration.declaration;
	}

	static bool AssertBuiltinType(Type* type, BuiltinTypeKind expectedKind)
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

	static bool AssertIdentifierExpression(Expression* expression, const char* expectedIdentifier)
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

		u64 expectedLength = strlen(expectedIdentifier);
		if (expression->identifier.name.Length() != expectedLength ||
		    memcmp(expression->identifier.name.Data(), expectedIdentifier, expectedLength) != 0)
		{
			WDT_RECORD_FAILURE("  expected identifier \"%s\", got \"%.*s\"\n", expectedIdentifier, (int)expression->identifier.name.Length(),
			                   expression->identifier.name.Data());
			return false;
		}

		return true;
	}

	static bool AssertIntegerConstant(Expression* expression, u64 expectedInteger)
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

	static bool AssertBooleanConstant(Expression* expression, bool expectedBoolean)
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

	static bool AssertFloatConstant(Expression* expression, f32 expectedFloat)
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

	static bool AssertDoubleConstant(Expression* expression, f64 expectedDouble)
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

	static bool AssertCastExpression(Expression* expression, BuiltinTypeKind expectedTargetKind)
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

	static bool AssertCallExpression(Expression* expression, const char* expectedFunctionName)
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

		u64 expectedLength = strlen(expectedFunctionName);
		if (expression->call.functionName.Length() != expectedLength ||
		    memcmp(expression->call.functionName.Data(), expectedFunctionName, expectedLength) != 0)
		{
			WDT_RECORD_FAILURE("  expected call target \"%s\", got \"%.*s\"\n", expectedFunctionName, (int)expression->call.functionName.Length(),
			                   expression->call.functionName.Data());
			return false;
		}

		if (expression->call.arguments.Length() != 0u)
		{
			WDT_RECORD_FAILURE("  expected call with 0 arguments, got %llu\n", expression->call.arguments.Length());
			return false;
		}

		return true;
	}

	static bool AssertCapturedDiagnostic(Diagnostics* diagnostics, u32 index, const ExpectedDiagnostic& expected)
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

	TEST(PackageDeclarationParsesEntrypointDirective)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = ParseSource(alloc, "package demo #entrypoint;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Statement* statement     = GetTopLevelStatement(&translationUnit, 0);
		Declaration* declaration = GetDeclarationFromStatement(statement);

		ASSERT_EQ(declaration->type, DECLARATION_TYPE_PACKAGE);
		ASSERT_STR_EQ(declaration->package.name, "demo");
		ASSERT_TRUE(declaration->package.isEntrypoint);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(PackageDeclarationParsesWithoutEntrypointDirective)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = ParseSource(alloc, "package demo;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Statement* statement     = GetTopLevelStatement(&translationUnit, 0);
		Declaration* declaration = GetDeclarationFromStatement(statement);

		ASSERT_EQ(declaration->type, DECLARATION_TYPE_PACKAGE);
		ASSERT_STR_EQ(declaration->package.name, "demo");
		ASSERT_FALSE(declaration->package.isEntrypoint);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(VariableDeclarationParsesBooleanInitializer)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = ParseSource(alloc, "bool flag = false;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Statement* statement     = GetTopLevelStatement(&translationUnit, 0);
		Declaration* declaration = GetDeclarationFromStatement(statement);

		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_STR_EQ(declaration->variable.name, "flag");
		if (!AssertBuiltinType(declaration->variable.type, BUILTIN_TYPE_BOOL))
			return;
		if (!AssertBooleanConstant(declaration->variable.initializer, false))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(VariableDeclarationParsesIdentifierInitializer)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = ParseSource(alloc, "int value = sourceValue;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Statement* statement     = GetTopLevelStatement(&translationUnit, 0);
		Declaration* declaration = GetDeclarationFromStatement(statement);

		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_STR_EQ(declaration->variable.name, "value");
		if (!AssertBuiltinType(declaration->variable.type, BUILTIN_TYPE_INT))
			return;
		if (!AssertIdentifierExpression(declaration->variable.initializer, "sourceValue"))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(VariableDeclarationParsesFloatInitializer)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = ParseSource(alloc, "float value = 3.25f;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Statement* statement     = GetTopLevelStatement(&translationUnit, 0);
		Declaration* declaration = GetDeclarationFromStatement(statement);

		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_STR_EQ(declaration->variable.name, "value");
		if (!AssertBuiltinType(declaration->variable.type, BUILTIN_TYPE_FLOAT))
			return;
		if (!AssertFloatConstant(declaration->variable.initializer, 3.25f))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(VariableDeclarationParsesDoubleInitializer)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = ParseSource(alloc, "double value = 6.5d;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Statement* statement     = GetTopLevelStatement(&translationUnit, 0);
		Declaration* declaration = GetDeclarationFromStatement(statement);

		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_STR_EQ(declaration->variable.name, "value");
		if (!AssertBuiltinType(declaration->variable.type, BUILTIN_TYPE_DOUBLE))
			return;
		if (!AssertDoubleConstant(declaration->variable.initializer, 6.5))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(VariableDeclarationsParseAllBuiltinTypes)
	{
		const BuiltinTypeCase cases[] = {
		    {"bool value = seed;", BUILTIN_TYPE_BOOL},       {"char value = seed;", BUILTIN_TYPE_CHAR},
		    {"uchar value = seed;", BUILTIN_TYPE_UCHAR},     {"short value = seed;", BUILTIN_TYPE_SHORT},
		    {"ushort value = seed;", BUILTIN_TYPE_USHORT},   {"int value = seed;", BUILTIN_TYPE_INT},
		    {"uint value = seed;", BUILTIN_TYPE_UINT},       {"long value = seed;", BUILTIN_TYPE_LONG},
		    {"ulong value = seed;", BUILTIN_TYPE_ULONG},     {"sz value = seed;", BUILTIN_TYPE_SZ},
		    {"usz value = seed;", BUILTIN_TYPE_USZ},         {"intptr value = seed;", BUILTIN_TYPE_INTPTR},
		    {"uintptr value = seed;", BUILTIN_TYPE_UINTPTR}, {"float value = seed;", BUILTIN_TYPE_FLOAT},
		    {"double value = seed;", BUILTIN_TYPE_DOUBLE},   {"string value = seed;", BUILTIN_TYPE_STRING},
		    {"cstring value = seed;", BUILTIN_TYPE_CSTRING}, {"rawptr value = seed;", BUILTIN_TYPE_RAWPTR},
		};

		for (u32 index = 0; index < ArraySize(cases); index++)
		{
			Diagnostics diag;
			TranslationUnit translationUnit = ParseSource(alloc, cases[index].source, &diag);

			ASSERT_EQ(translationUnit.statements.Length(), 1u);

			Statement* statement     = GetTopLevelStatement(&translationUnit, 0);
			Declaration* declaration = GetDeclarationFromStatement(statement);

			ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
			ASSERT_STR_EQ(declaration->variable.name, "value");
			if (!AssertBuiltinType(declaration->variable.type, cases[index].expectedKind))
				return;
			if (!AssertIdentifierExpression(declaration->variable.initializer, "seed"))
				return;
			ASSERT_NO_DIAGNOSTICS(diag);
		}
	}

	TEST(FunctionDeclarationParsesBlockStatements)
	{
		Diagnostics diag;
		const char* source              = "fn int main() { int value = helper(); return value; }";
		TranslationUnit translationUnit = ParseSource(alloc, source, &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Statement* statement     = GetTopLevelStatement(&translationUnit, 0);
		Declaration* declaration = GetDeclarationFromStatement(statement);

		ASSERT_EQ(declaration->type, DECLARATION_TYPE_FUNCTION);
		ASSERT_STR_EQ(declaration->function.name, "main");
		if (!AssertBuiltinType(declaration->function.returnType, BUILTIN_TYPE_INT))
			return;
		ASSERT_EQ(declaration->function.parameters.Length(), 0u);

		Statement* body = declaration->function.body;
		ASSERT_EQ(body->type, STATEMENT_TYPE_BLOCK);
		ASSERT_EQ(body->block.statements.Length(), 2u);

		Statement* variableStatement = body->block.statements[0];
		ASSERT_EQ(variableStatement->type, STATEMENT_TYPE_DECLARATION);

		Declaration* variableDeclaration = variableStatement->declaration.declaration;
		ASSERT_EQ(variableDeclaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_STR_EQ(variableDeclaration->variable.name, "value");
		if (!AssertBuiltinType(variableDeclaration->variable.type, BUILTIN_TYPE_INT))
			return;
		if (!AssertCallExpression(variableDeclaration->variable.initializer, "helper"))
			return;

		Statement* returnStatement = body->block.statements[1];
		ASSERT_EQ(returnStatement->type, STATEMENT_TYPE_RETURN);
		if (!AssertIdentifierExpression(returnStatement->returnStmt.expression, "value"))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(FunctionDeclarationParsesVoidReturnTypeAndEmptyBody)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = ParseSource(alloc, "fn void main() {}", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Statement* statement     = GetTopLevelStatement(&translationUnit, 0);
		Declaration* declaration = GetDeclarationFromStatement(statement);

		ASSERT_EQ(declaration->type, DECLARATION_TYPE_FUNCTION);
		ASSERT_STR_EQ(declaration->function.name, "main");
		if (!AssertBuiltinType(declaration->function.returnType, BUILTIN_TYPE_VOID))
			return;
		ASSERT_EQ(declaration->function.parameters.Length(), 0u);
		ASSERT_EQ(declaration->function.body->type, STATEMENT_TYPE_BLOCK);
		ASSERT_EQ(declaration->function.body->block.statements.Length(), 0u);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(FunctionDeclarationParsesMultilineBodyWithComments)
	{
		Diagnostics diag;
		const char* source              = "fn int main()\n"
		                                  "{\n"
		                                  "\t<* keep this ignored *>\n"
		                                  "\thelper();\n"
		                                  "\t// return the previous result\n"
		                                  "\treturn value;\n"
		                                  "}";
		TranslationUnit translationUnit = ParseSource(alloc, source, &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Statement* statement     = GetTopLevelStatement(&translationUnit, 0);
		Declaration* declaration = GetDeclarationFromStatement(statement);

		ASSERT_EQ(declaration->type, DECLARATION_TYPE_FUNCTION);
		ASSERT_STR_EQ(declaration->function.name, "main");
		if (!AssertBuiltinType(declaration->function.returnType, BUILTIN_TYPE_INT))
			return;
		ASSERT_EQ(declaration->function.body->type, STATEMENT_TYPE_BLOCK);
		ASSERT_EQ(declaration->function.body->block.statements.Length(), 2u);

		Statement* expressionStatement = declaration->function.body->block.statements[0];
		ASSERT_EQ(expressionStatement->type, STATEMENT_TYPE_EXPRESSION);
		if (!AssertCallExpression(expressionStatement->expression.expression, "helper"))
			return;

		Statement* returnStatement = declaration->function.body->block.statements[1];
		ASSERT_EQ(returnStatement->type, STATEMENT_TYPE_RETURN);
		if (!AssertIdentifierExpression(returnStatement->returnStmt.expression, "value"))
			return;

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(TopLevelCallExpressionStatementParses)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = ParseSource(alloc, "main();", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Statement* statement = GetTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(statement->type, STATEMENT_TYPE_EXPRESSION);
		if (!AssertCallExpression(statement->expression.expression, "main"))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(CallExpressionStatementParsesInsideFunctionBody)
	{
		Diagnostics diag;
		const char* source              = "fn int main() { helper(); return value; }";
		TranslationUnit translationUnit = ParseSource(alloc, source, &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Statement* statement     = GetTopLevelStatement(&translationUnit, 0);
		Declaration* declaration = GetDeclarationFromStatement(statement);

		ASSERT_EQ(declaration->type, DECLARATION_TYPE_FUNCTION);
		ASSERT_EQ(declaration->function.body->type, STATEMENT_TYPE_BLOCK);
		ASSERT_EQ(declaration->function.body->block.statements.Length(), 2u);

		Statement* expressionStatement = declaration->function.body->block.statements[0];
		ASSERT_EQ(expressionStatement->type, STATEMENT_TYPE_EXPRESSION);
		if (!AssertCallExpression(expressionStatement->expression.expression, "helper"))
			return;

		Statement* returnStatement = declaration->function.body->block.statements[1];
		ASSERT_EQ(returnStatement->type, STATEMENT_TYPE_RETURN);
		if (!AssertIdentifierExpression(returnStatement->returnStmt.expression, "value"))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(CastExpressionParsesWithIdentifierOperand)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = ParseSource(alloc, "float y = cast(float) x;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Statement* statement     = GetTopLevelStatement(&translationUnit, 0);
		Declaration* declaration = GetDeclarationFromStatement(statement);

		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_STR_EQ(declaration->variable.name, "y");
		if (!AssertBuiltinType(declaration->variable.type, BUILTIN_TYPE_FLOAT))
			return;
		if (!AssertCastExpression(declaration->variable.initializer, BUILTIN_TYPE_FLOAT))
			return;
		if (!AssertIdentifierExpression(declaration->variable.initializer->cast.expression, "x"))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(CastExpressionParsesWithConstantOperand)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = ParseSource(alloc, "int value = cast(int) 12;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromStatement(GetTopLevelStatement(&translationUnit, 0));
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		if (!AssertCastExpression(declaration->variable.initializer, BUILTIN_TYPE_INT))
			return;
		if (!AssertIntegerConstant(declaration->variable.initializer->cast.expression, 12u))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(CastExpressionAppliesToCallResult)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = ParseSource(alloc, "int value = cast(int) helper();", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromStatement(GetTopLevelStatement(&translationUnit, 0));
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		if (!AssertCastExpression(declaration->variable.initializer, BUILTIN_TYPE_INT))
			return;
		if (!AssertCallExpression(declaration->variable.initializer->cast.expression, "helper"))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(CastExpressionMissingOpenParenReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int v = cast 12;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 14u, "Expected a '('"}))
			return;
	}

	TEST(CastExpressionMissingTargetTypeReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int v = cast() 12;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 14u, "Expected a type"}))
			return;
	}

	TEST(CastExpressionMissingCloseParenReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int v = cast(int 12;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 18u, "Expected a ')'"}))
			return;
	}

	TEST(CastExpressionMissingOperandReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int v = cast(int);", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 18u, "Expected an expression"}))
			return;
	}

	TEST(TopLevelIntegerLiteralStatementReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "42;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 1u, "Expected a top-level statement"}))
			return;
	}

	TEST(BooleanLiteralStatementInFunctionBodyReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "fn int main() { true; }", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		Declaration* functionDeclaration = GetDeclarationFromStatement(translationUnit.statements[0]);
		ASSERT_EQ(functionDeclaration->type, DECLARATION_TYPE_FUNCTION);
		ASSERT_EQ(functionDeclaration->function.body->type, STATEMENT_TYPE_BLOCK);
		ASSERT_EQ(functionDeclaration->function.body->block.statements.Length(), 0u);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 17u, "Expected a statement"}))
			return;
	}

	TEST(MultipleTopLevelDeclarationsPreserveOrder)
	{
		Diagnostics diag;
		const char* source              = "package demo; int answer = 42; fn int main() { return answer; }";
		TranslationUnit translationUnit = ParseSource(alloc, source, &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 3u);

		Declaration* packageDeclaration = GetDeclarationFromStatement(GetTopLevelStatement(&translationUnit, 0));
		ASSERT_EQ(packageDeclaration->type, DECLARATION_TYPE_PACKAGE);
		ASSERT_STR_EQ(packageDeclaration->package.name, "demo");
		ASSERT_FALSE(packageDeclaration->package.isEntrypoint);

		Declaration* variableDeclaration = GetDeclarationFromStatement(GetTopLevelStatement(&translationUnit, 1));
		ASSERT_EQ(variableDeclaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_STR_EQ(variableDeclaration->variable.name, "answer");
		if (!AssertBuiltinType(variableDeclaration->variable.type, BUILTIN_TYPE_INT))
			return;
		if (!AssertIntegerConstant(variableDeclaration->variable.initializer, 42))
			return;

		Declaration* functionDeclaration = GetDeclarationFromStatement(GetTopLevelStatement(&translationUnit, 2));
		ASSERT_EQ(functionDeclaration->type, DECLARATION_TYPE_FUNCTION);
		ASSERT_STR_EQ(functionDeclaration->function.name, "main");
		ASSERT_EQ(functionDeclaration->function.body->type, STATEMENT_TYPE_BLOCK);
		ASSERT_EQ(functionDeclaration->function.body->block.statements.Length(), 1u);
		ASSERT_EQ(functionDeclaration->function.body->block.statements[0]->type, STATEMENT_TYPE_RETURN);
		if (!AssertIdentifierExpression(functionDeclaration->function.body->block.statements[0]->returnStmt.expression, "answer"))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(MissingPackageIdentifierReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "package ;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 9u, "Expected an identifier"}))
			return;
	}

	TEST(DuplicateEntrypointDirectiveReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "package demo #entrypoint #entrypoint;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 26u, "Duplicate '#entrypoint' directive"}))
			return;
	}

	TEST(MissingSemicolonAfterPackageDeclarationReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "package demo", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 13u, "Expected a ';'"}))
			return;
	}

	TEST(MissingSemicolonAfterVariableDeclarationReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int value = 42", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 15u, "Expected a ';'"}))
			return;
	}

	TEST(MissingSemicolonAfterVariableDeclarationReportsDiagnosticOnLaterLine)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source              = "package demo;\n"
		                                  "int value = 42";
		TranslationUnit translationUnit = ParseSource(alloc, source, &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 2u);
		Declaration* packageDeclaration = GetDeclarationFromStatement(GetTopLevelStatement(&translationUnit, 0));
		ASSERT_EQ(packageDeclaration->type, DECLARATION_TYPE_PACKAGE);
		ASSERT_EQ(translationUnit.statements[1]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 15u, "Expected a ';'"}))
			return;
	}

	TEST(MissingVariableIdentifierReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int = 1;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 5u, "Expected an identifier"}))
			return;
	}

	TEST(MissingVariableInitializerReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int value = ;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 13u, "Expected an expression"}))
			return;
	}

	TEST(MissingFunctionNameReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "fn int () {}", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 8u, "Expected an identifier"}))
			return;
	}

	TEST(FunctionWithParametersReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "fn int main(int value) { return value; }", &diag);

		ASSERT_TRUE(translationUnit.statements.Length() >= 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_TRUE(diag.CapturedCount() >= 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 13u, "Expected a ')'"}))
			return;
	}

	TEST(MissingFunctionBodyReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "fn int main();", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 14u, "Expected a '{' to start a scope"}))
			return;
	}

	TEST(ReturnStatementMissingExpressionReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "fn int main() { return; }", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		Declaration* functionDeclaration = GetDeclarationFromStatement(translationUnit.statements[0]);
		ASSERT_EQ(functionDeclaration->type, DECLARATION_TYPE_FUNCTION);
		ASSERT_EQ(functionDeclaration->function.body->type, STATEMENT_TYPE_BLOCK);
		ASSERT_EQ(functionDeclaration->function.body->block.statements.Length(), 0u);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 23u, "Expected an expression"}))
			return;
	}

	TEST(MissingReturnSemicolonReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "fn int main() { return value }", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		Declaration* functionDeclaration = GetDeclarationFromStatement(translationUnit.statements[0]);
		ASSERT_EQ(functionDeclaration->type, DECLARATION_TYPE_FUNCTION);
		ASSERT_EQ(functionDeclaration->function.body->type, STATEMENT_TYPE_BLOCK);
		ASSERT_EQ(functionDeclaration->function.body->block.statements.Length(), 0u);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 30u, "Expected a ';'"}))
			return;
	}

	TEST(RecoveryContinuesInsideFunctionBodyAfterInvalidStatement)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source              = "fn int main() {\n"
		                                  "    int x = 12;\n"
		                                  "    usz y = 15;\n"
		                                  "    float = 12;\n"
		                                  "    return y;\n"
		                                  "}";
		TranslationUnit translationUnit = ParseSource(alloc, source, &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* functionDeclaration = GetDeclarationFromStatement(translationUnit.statements[0]);
		ASSERT_EQ(functionDeclaration->type, DECLARATION_TYPE_FUNCTION);
		ASSERT_STR_EQ(functionDeclaration->function.name, "main");
		ASSERT_EQ(functionDeclaration->function.body->type, STATEMENT_TYPE_BLOCK);
		ASSERT_EQ(functionDeclaration->function.body->block.statements.Length(), 3u);
		ASSERT_EQ(functionDeclaration->function.body->block.statements[0]->type, STATEMENT_TYPE_DECLARATION);
		ASSERT_EQ(functionDeclaration->function.body->block.statements[1]->type, STATEMENT_TYPE_DECLARATION);
		ASSERT_EQ(functionDeclaration->function.body->block.statements[2]->type, STATEMENT_TYPE_RETURN);
		if (!AssertIdentifierExpression(functionDeclaration->function.body->block.statements[2]->returnStmt.expression, "y"))
			return;

		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 4u, 11u, "Expected an identifier"}))
			return;
	}

	TEST(InvalidCallTargetReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int value = 42();", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 13u, "Call target must be an identifier"}))
			return;
	}

	TEST(MissingCallCloseParenReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int value = helper(; ", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 20u, "Expected a ')'"}))
			return;
	}

	TEST(NumericLiteralOutOfRangeReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int value = 18446744073709551616;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 13u, "does not fit in integer"}))
			return;
	}

	TEST(RecoveryContinuesAfterInvalidTopLevelStatement)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int = 1; package demo;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 2u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);

		Declaration* packageDeclaration = GetDeclarationFromStatement(translationUnit.statements[1]);
		ASSERT_EQ(packageDeclaration->type, DECLARATION_TYPE_PACKAGE);
		ASSERT_STR_EQ(packageDeclaration->package.name, "demo");

		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 5u, "Expected an identifier"}))
			return;
	}

	TEST(RecoveryContinuesAfterMultipleConsecutiveInvalidStatementsInBlock)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		const char* source              = "fn int main() {\n"
		                                  "    float = 1;\n"
		                                  "    int = 2;\n"
		                                  "    int value = 3;\n"
		                                  "    return value;\n"
		                                  "}";
		TranslationUnit translationUnit = ParseSource(alloc, source, &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* functionDeclaration = GetDeclarationFromStatement(translationUnit.statements[0]);
		ASSERT_EQ(functionDeclaration->type, DECLARATION_TYPE_FUNCTION);
		ASSERT_EQ(functionDeclaration->function.body->type, STATEMENT_TYPE_BLOCK);
		ASSERT_EQ(functionDeclaration->function.body->block.statements.Length(), 2u);
		ASSERT_EQ(functionDeclaration->function.body->block.statements[0]->type, STATEMENT_TYPE_DECLARATION);
		ASSERT_EQ(functionDeclaration->function.body->block.statements[1]->type, STATEMENT_TYPE_RETURN);

		ASSERT_EQ(diag.CapturedCount(), 2u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 11u, "Expected an identifier"}))
			return;
		if (!AssertCapturedDiagnostic(&diag, 1, {Diagnostics::Severity::Error, 3u, 9u, "Expected an identifier"}))
			return;
	}

	TEST(RecoveryContinuesAfterInvalidFunctionDeclaration)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "fn int main( { } package demo;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 2u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);

		Declaration* packageDeclaration = GetDeclarationFromStatement(translationUnit.statements[1]);
		ASSERT_EQ(packageDeclaration->type, DECLARATION_TYPE_PACKAGE);
		ASSERT_STR_EQ(packageDeclaration->package.name, "demo");

		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 14u, "Expected a ')'"}))
			return;
	}

	TestResults RunParserTests()
	{
		ResetTestCounters();

		HeapAllocator heap;
		ArenaAllocator arena(&heap, Megabytes(4));

		ScopedTimer timer;

		printf("%sRunning parser tests...%s\n", TestColor(ANSI_COLOR_BOLD), TestColor(ANSI_COLOR_RESET));

		PrintSection("Declarations and statements");
		RUN_TEST(PackageDeclarationParsesEntrypointDirective);
		RUN_TEST(PackageDeclarationParsesWithoutEntrypointDirective);
		RUN_TEST(VariableDeclarationParsesBooleanInitializer);
		RUN_TEST(VariableDeclarationParsesIdentifierInitializer);
		RUN_TEST(VariableDeclarationParsesFloatInitializer);
		RUN_TEST(VariableDeclarationParsesDoubleInitializer);
		RUN_TEST(VariableDeclarationsParseAllBuiltinTypes);
		RUN_TEST(FunctionDeclarationParsesBlockStatements);
		RUN_TEST(FunctionDeclarationParsesVoidReturnTypeAndEmptyBody);
		RUN_TEST(FunctionDeclarationParsesMultilineBodyWithComments);
		RUN_TEST(TopLevelCallExpressionStatementParses);
		RUN_TEST(CallExpressionStatementParsesInsideFunctionBody);
		RUN_TEST(MultipleTopLevelDeclarationsPreserveOrder);
		RUN_TEST(CastExpressionParsesWithIdentifierOperand);
		RUN_TEST(CastExpressionParsesWithConstantOperand);
		RUN_TEST(CastExpressionAppliesToCallResult);

		PrintSection("Diagnostics and recovery");
		RUN_TEST(MissingPackageIdentifierReportsDiagnostic);
		RUN_TEST(DuplicateEntrypointDirectiveReportsDiagnostic);
		RUN_TEST(MissingSemicolonAfterPackageDeclarationReportsDiagnostic);
		RUN_TEST(MissingSemicolonAfterVariableDeclarationReportsDiagnostic);
		RUN_TEST(MissingSemicolonAfterVariableDeclarationReportsDiagnosticOnLaterLine);
		RUN_TEST(MissingVariableIdentifierReportsDiagnostic);
		RUN_TEST(MissingVariableInitializerReportsDiagnostic);
		RUN_TEST(MissingFunctionNameReportsDiagnostic);
		RUN_TEST(FunctionWithParametersReportsDiagnostic);
		RUN_TEST(MissingFunctionBodyReportsDiagnostic);
		RUN_TEST(ReturnStatementMissingExpressionReportsDiagnostic);
		RUN_TEST(MissingReturnSemicolonReportsDiagnostic);
		RUN_TEST(TopLevelIntegerLiteralStatementReportsDiagnostic);
		RUN_TEST(BooleanLiteralStatementInFunctionBodyReportsDiagnostic);
		RUN_TEST(InvalidCallTargetReportsDiagnostic);
		RUN_TEST(MissingCallCloseParenReportsDiagnostic);
		RUN_TEST(NumericLiteralOutOfRangeReportsDiagnostic);
		RUN_TEST(CastExpressionMissingOpenParenReportsDiagnostic);
		RUN_TEST(CastExpressionMissingTargetTypeReportsDiagnostic);
		RUN_TEST(CastExpressionMissingCloseParenReportsDiagnostic);
		RUN_TEST(CastExpressionMissingOperandReportsDiagnostic);
		RUN_TEST(RecoveryContinuesInsideFunctionBodyAfterInvalidStatement);
		RUN_TEST(RecoveryContinuesAfterMultipleConsecutiveInvalidStatementsInBlock);
		RUN_TEST(RecoveryContinuesAfterInvalidTopLevelStatement);
		RUN_TEST(RecoveryContinuesAfterInvalidFunctionDeclaration);

		f64 totalMs = timer.GetElapsedMilliseconds();
		return PrintTestSummary("Parser", totalMs);
	}

} // namespace Wandelt
