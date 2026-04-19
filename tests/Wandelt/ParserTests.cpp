#include "ParserTests.hpp"

#include "Wandelt/Parser.hpp"

namespace Wandelt
{

	struct BuiltinTypeCase
	{
		const char* source;
		BuiltinTypeKind expectedKind;
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

	static void AssertBuiltinType(Type* type, BuiltinTypeKind expectedKind)
	{
		ASSERT_TRUE(type != nullptr);
		ASSERT_EQ(type->kind, TYPE_KIND_BUILTIN);
		ASSERT_EQ(type->basic.kind, expectedKind);
	}

	static void AssertIdentifierExpression(Expression* expression, const char* expectedIdentifier)
	{
		ASSERT_TRUE(expression != nullptr);
		ASSERT_EQ(expression->type, EXPRESSION_TYPE_IDENTIFIER);
		ASSERT_STR_EQ(expression->identifier.name, expectedIdentifier);
	}

	static void AssertIntegerConstant(Expression* expression, u64 expectedInteger)
	{
		ASSERT_TRUE(expression != nullptr);
		ASSERT_EQ(expression->type, EXPRESSION_TYPE_CONSTANT);
		ASSERT_EQ(expression->constant.kind, CONSTANT_KIND_INTEGER);
		ASSERT_EQ(expression->constant.integerValue, expectedInteger);
	}

	static void AssertBooleanConstant(Expression* expression, bool expectedBoolean)
	{
		ASSERT_TRUE(expression != nullptr);
		ASSERT_EQ(expression->type, EXPRESSION_TYPE_CONSTANT);
		ASSERT_EQ(expression->constant.kind, CONSTANT_KIND_BOOLEAN);
		ASSERT_EQ(expression->constant.booleanValue, expectedBoolean);
	}

	static void AssertFloatConstant(Expression* expression, f32 expectedFloat)
	{
		const f32 tolerance = 0.0001f;

		ASSERT_TRUE(expression != nullptr);
		ASSERT_EQ(expression->type, EXPRESSION_TYPE_CONSTANT);
		ASSERT_EQ(expression->constant.kind, CONSTANT_KIND_FLOAT);
		ASSERT_TRUE(expression->constant.floatValue >= expectedFloat - tolerance);
		ASSERT_TRUE(expression->constant.floatValue <= expectedFloat + tolerance);
	}

	static void AssertDoubleConstant(Expression* expression, f64 expectedDouble)
	{
		const f64 tolerance = 0.0000001;

		ASSERT_TRUE(expression != nullptr);
		ASSERT_EQ(expression->type, EXPRESSION_TYPE_CONSTANT);
		ASSERT_EQ(expression->constant.kind, CONSTANT_KIND_DOUBLE);
		ASSERT_TRUE(expression->constant.doubleValue >= expectedDouble - tolerance);
		ASSERT_TRUE(expression->constant.doubleValue <= expectedDouble + tolerance);
	}

	static void AssertCallExpression(Expression* expression, const char* expectedFunctionName)
	{
		ASSERT_TRUE(expression != nullptr);
		ASSERT_EQ(expression->type, EXPRESSION_TYPE_CALL);
		ASSERT_STR_EQ(expression->call.functionName, expectedFunctionName);
		ASSERT_EQ(expression->call.arguments.Length(), 0u);
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
		AssertBuiltinType(declaration->variable.type, BUILTIN_TYPE_BOOL);
		AssertBooleanConstant(declaration->variable.initializer, false);
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
		AssertBuiltinType(declaration->variable.type, BUILTIN_TYPE_INT);
		AssertIdentifierExpression(declaration->variable.initializer, "sourceValue");
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
		AssertBuiltinType(declaration->variable.type, BUILTIN_TYPE_FLOAT);
		AssertFloatConstant(declaration->variable.initializer, 3.25f);
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
		AssertBuiltinType(declaration->variable.type, BUILTIN_TYPE_DOUBLE);
		AssertDoubleConstant(declaration->variable.initializer, 6.5);
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
			AssertBuiltinType(declaration->variable.type, cases[index].expectedKind);
			AssertIdentifierExpression(declaration->variable.initializer, "seed");
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
		AssertBuiltinType(declaration->function.returnType, BUILTIN_TYPE_INT);
		ASSERT_EQ(declaration->function.parameters.Length(), 0u);

		Statement* body = declaration->function.body;
		ASSERT_EQ(body->type, STATEMENT_TYPE_BLOCK);
		ASSERT_EQ(body->block.statements.Length(), 2u);

		Statement* variableStatement = body->block.statements[0];
		ASSERT_EQ(variableStatement->type, STATEMENT_TYPE_DECLARATION);

		Declaration* variableDeclaration = variableStatement->declaration.declaration;
		ASSERT_EQ(variableDeclaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_STR_EQ(variableDeclaration->variable.name, "value");
		AssertBuiltinType(variableDeclaration->variable.type, BUILTIN_TYPE_INT);
		AssertCallExpression(variableDeclaration->variable.initializer, "helper");

		Statement* returnStatement = body->block.statements[1];
		ASSERT_EQ(returnStatement->type, STATEMENT_TYPE_RETURN);
		AssertIdentifierExpression(returnStatement->returnStmt.expression, "value");
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
		AssertBuiltinType(declaration->function.returnType, BUILTIN_TYPE_VOID);
		ASSERT_EQ(declaration->function.parameters.Length(), 0u);
		ASSERT_EQ(declaration->function.body->type, STATEMENT_TYPE_BLOCK);
		ASSERT_EQ(declaration->function.body->block.statements.Length(), 0u);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(TopLevelCallExpressionStatementParses)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = ParseSource(alloc, "main();", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Statement* statement = GetTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(statement->type, STATEMENT_TYPE_EXPRESSION);
		AssertCallExpression(statement->expression.expression, "main");
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
		AssertCallExpression(expressionStatement->expression.expression, "helper");

		Statement* returnStatement = declaration->function.body->block.statements[1];
		ASSERT_EQ(returnStatement->type, STATEMENT_TYPE_RETURN);
		AssertIdentifierExpression(returnStatement->returnStmt.expression, "value");
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(TopLevelIntegerLiteralStatementReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "42;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected a top-level statement");
	}

	TEST(BooleanLiteralStatementInFunctionBodyReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "fn int main() { true; }", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 2u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(translationUnit.statements[1]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 2u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected a statement");
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
		AssertBuiltinType(variableDeclaration->variable.type, BUILTIN_TYPE_INT);
		AssertIntegerConstant(variableDeclaration->variable.initializer, 42);

		Declaration* functionDeclaration = GetDeclarationFromStatement(GetTopLevelStatement(&translationUnit, 2));
		ASSERT_EQ(functionDeclaration->type, DECLARATION_TYPE_FUNCTION);
		ASSERT_STR_EQ(functionDeclaration->function.name, "main");
		ASSERT_EQ(functionDeclaration->function.body->type, STATEMENT_TYPE_BLOCK);
		ASSERT_EQ(functionDeclaration->function.body->block.statements.Length(), 1u);
		ASSERT_EQ(functionDeclaration->function.body->block.statements[0]->type, STATEMENT_TYPE_RETURN);
		AssertIdentifierExpression(functionDeclaration->function.body->block.statements[0]->returnStmt.expression, "answer");
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

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected an identifier");
	}

	TEST(DuplicateEntrypointDirectiveReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "package demo #entrypoint #entrypoint;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Duplicate '#entrypoint' directive");
	}

	TEST(MissingSemicolonAfterPackageDeclarationReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "package demo", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected a ';'");
	}

	TEST(MissingSemicolonAfterVariableDeclarationReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int value = 42", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected a ';'");
	}

	TEST(MissingVariableIdentifierReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int = 1;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected an identifier");
	}

	TEST(MissingVariableInitializerReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int value = ;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected an expression");
	}

	TEST(MissingFunctionNameReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "fn int () {}", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected an identifier");
	}

	TEST(FunctionWithParametersReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "fn int main(int value) { return value; }", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 2u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(translationUnit.statements[1]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 2u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected a ')'");
	}

	TEST(MissingFunctionBodyReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "fn int main();", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected a '{' to start a scope");
	}

	TEST(ReturnStatementMissingExpressionReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "fn int main() { return; }", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 2u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(translationUnit.statements[1]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 2u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected an expression");
	}

	TEST(MissingReturnSemicolonReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "fn int main() { return value }", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected a ';'");
	}

	TEST(InvalidCallTargetReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int value = 42();", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Call target must be an identifier");
	}

	TEST(MissingCallCloseParenReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int value = helper(; ", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected a ')'");
	}

	TEST(NumericLiteralOutOfRangeReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = ParseSource(alloc, "int value = 18446744073709551616;", &diag);

		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(translationUnit.statements[0]->type, STATEMENT_TYPE_INVALID);
		ASSERT_EQ(diag.CapturedCount(), 1u);

		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "does not fit in integer");
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
		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected an identifier");
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
		Diagnostics::Entry* entry = diag.GetCaptured(0);
		ASSERT_EQ(entry->severity, Diagnostics::Severity::Error);
		ASSERT_STR_CONTAINS(entry->message, "Expected a ')'");
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
		RUN_TEST(TopLevelCallExpressionStatementParses);
		RUN_TEST(CallExpressionStatementParsesInsideFunctionBody);
		RUN_TEST(MultipleTopLevelDeclarationsPreserveOrder);

		PrintSection("Diagnostics and recovery");
		RUN_TEST(MissingPackageIdentifierReportsDiagnostic);
		RUN_TEST(DuplicateEntrypointDirectiveReportsDiagnostic);
		RUN_TEST(MissingSemicolonAfterPackageDeclarationReportsDiagnostic);
		RUN_TEST(MissingSemicolonAfterVariableDeclarationReportsDiagnostic);
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
		RUN_TEST(RecoveryContinuesAfterInvalidTopLevelStatement);
		RUN_TEST(RecoveryContinuesAfterInvalidFunctionDeclaration);

		f64 totalMs = timer.GetElapsedMilliseconds();
		return PrintTestSummary("Parser", totalMs);
	}

} // namespace Wandelt
