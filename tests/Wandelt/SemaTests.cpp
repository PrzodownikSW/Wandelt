#include "SemaTests.hpp"

#include "Wandelt/Parser.hpp"
#include "Wandelt/Sema.hpp"

namespace Wandelt
{

	struct ExpectedDiagnostic
	{
		Diagnostics::Severity severity;
		u32 line;
		u32 col;
		const char* messageSubstring;
	};

	static bool ParseAndAnalyzeSource(Allocator* alloc, const char* source, Diagnostics* diagnostics, TranslationUnit* outTranslationUnit)
	{
		File file = MakeTestFile(alloc, source);
		Lexer lexer{&file, diagnostics};
		Parser parser{alloc, alloc, alloc, &lexer, diagnostics};

		*outTranslationUnit = parser.Parse();
		if (diagnostics->HasErrors())
			return false;

		Sema sema{alloc, alloc, outTranslationUnit, diagnostics};
		return sema.Analyze();
	}

	static Declaration* GetDeclarationFromTopLevelStatement(TranslationUnit* translationUnit, u64 index)
	{
		ASSERT(index < translationUnit->statements.Length(), "top-level statement index out of bounds");

		Statement* statement = translationUnit->statements[index];
		ASSERT(statement->type == STATEMENT_TYPE_DECLARATION, "statement is not a declaration");
		return statement->declaration.declaration;
	}

	static Statement* GetFunctionBodyStatement(Declaration* declaration, u64 index)
	{
		ASSERT(declaration != nullptr, "declaration is null");
		ASSERT(declaration->type == DECLARATION_TYPE_FUNCTION, "declaration is not a function");
		ASSERT(declaration->function.body != nullptr, "function body is null");
		ASSERT(declaration->function.body->type == STATEMENT_TYPE_BLOCK, "function body is not a block");
		ASSERT(index < declaration->function.body->block.statements.Length(), "function body statement index out of bounds");

		return declaration->function.body->block.statements[index];
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

	TEST(IntegerConstantAdoptsHintedFloatType)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "float value = 12;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_EQ(declaration->variable.initializer->type, EXPRESSION_TYPE_CONSTANT);
		ASSERT_EQ(declaration->variable.initializer->constant.kind, CONSTANT_KIND_INTEGER);
		if (!AssertBuiltinType(declaration->variable.initializer->resolvedType, BUILTIN_TYPE_FLOAT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(IntegerConstantAdoptsHintedDoubleType)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "double value = 12;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_EQ(declaration->variable.initializer->type, EXPRESSION_TYPE_CONSTANT);
		ASSERT_EQ(declaration->variable.initializer->constant.kind, CONSTANT_KIND_INTEGER);
		if (!AssertBuiltinType(declaration->variable.initializer->resolvedType, BUILTIN_TYPE_DOUBLE))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(IntegerConstantAdoptsHintedULongType)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "ulong value = 18446744073709551615;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_EQ(declaration->variable.initializer->type, EXPRESSION_TYPE_CONSTANT);
		ASSERT_EQ(declaration->variable.initializer->constant.kind, CONSTANT_KIND_INTEGER);
		if (!AssertBuiltinType(declaration->variable.initializer->resolvedType, BUILTIN_TYPE_ULONG))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(SmallIntegerConstantDefaultsToIntWithoutHint)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "long value = cast(long) 12;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_EQ(declaration->variable.initializer->type, EXPRESSION_TYPE_CAST);
		ASSERT_EQ(declaration->variable.initializer->cast.expression->type, EXPRESSION_TYPE_CONSTANT);
		if (!AssertBuiltinType(declaration->variable.initializer->cast.expression->resolvedType, BUILTIN_TYPE_INT))
			return;
		ASSERT_EQ(diag.ErrorCount(), 0u);
		ASSERT_EQ(diag.WarningCount(), 1u);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Warning, 1u, 14u, "Unnecessary cast"}))
			return;
	}

	TEST(LargeIntegerConstantDefaultsToAbstractIntegerWithoutHint)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "long value = cast(long) 5000000000;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_EQ(declaration->variable.initializer->type, EXPRESSION_TYPE_CAST);
		ASSERT_EQ(declaration->variable.initializer->cast.expression->type, EXPRESSION_TYPE_CONSTANT);
		if (!AssertBuiltinType(declaration->variable.initializer->cast.expression->resolvedType, BUILTIN_TYPE_ABSTRACT_INT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(IntegerConstantTooLargeForSignedHintReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "long value = 18446744073709551615;", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 14u, "expected type 'long'"}))
			return;
	}

	TEST(IntegerConstantMustFitHintedFloatExactly)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "float value = 16777217;", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 15u, "expected type 'float'"}))
			return;
	}

	TEST(ReturnStatementImplicitlyCastsToFunctionReturnType)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn long widen() {\n"
		                                                                          "    char value = 12;\n"
		                                                                          "    return value;\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* functionDeclaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(functionDeclaration->type, DECLARATION_TYPE_FUNCTION);
		Statement* returnStatement = GetFunctionBodyStatement(functionDeclaration, 1);
		ASSERT_EQ(returnStatement->type, STATEMENT_TYPE_RETURN);
		if (!AssertCastExpression(returnStatement->returnStmt.expression, BUILTIN_TYPE_LONG))
			return;
		if (!AssertBuiltinType(returnStatement->returnStmt.expression->cast.expression->resolvedType, BUILTIN_TYPE_CHAR))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(ReturnStatementTypeMismatchReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int main() {\n"
		                                                                          "    bool flag = true;\n"
		                                                                          "    return flag;\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 3u, 5u, "return expression of type 'bool'"}))
			return;
	}

	TEST(VoidFunctionReturningValueReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn void main() {\n"
		                                                                          "    bool flag = true;\n"
		                                                                          "    return flag;\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 3u, 5u, "return expression of type 'bool'"}))
			return;
	}

	TEST(NonVoidFunctionWithoutReturnReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn int main() { int value = 12; }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 1u, "must end with a return statement"}))
			return;
	}

	TEST(VariableInitializerIdentifierImplicitlyCasts)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "char small = 12; int value = small;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 2u);

		Declaration* valueDeclaration = GetDeclarationFromTopLevelStatement(&translationUnit, 1);
		ASSERT_EQ(valueDeclaration->type, DECLARATION_TYPE_VARIABLE);
		if (!AssertCastExpression(valueDeclaration->variable.initializer, BUILTIN_TYPE_INT))
			return;
		if (!AssertBuiltinType(valueDeclaration->variable.initializer->cast.expression->resolvedType, BUILTIN_TYPE_CHAR))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(BoolVariableRejectsIntegerLiteralInitializer)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "bool value = 1;", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 14u, "expected type 'bool'"}))
			return;
	}

	TEST(DoubleVariableAdoptsFloatLiteralInitializer)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "double value = 3.25f;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		if (!AssertBuiltinType(declaration->variable.initializer->resolvedType, BUILTIN_TYPE_DOUBLE))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(FloatVariableRejectsDoubleLiteralInitializer)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "float value = 3.25d;", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 15u, "expected type 'float'"}))
			return;
	}

	TEST(UndeclaredIdentifierInInitializerReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "int value = missing;", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 13u, "Undeclared identifier 'missing'"}))
			return;
	}

	TEST(SelfReferentialVariableReportsCycleDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "int value = value;", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0,
		                              {Diagnostics::Severity::Error, 1u, 13u, "Cyclic dependency detected while resolving identifier 'value'"}))
			return;
	}

	TEST(DuplicateFunctionDeclarationReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int first() { return 1; }\n"
		                                                                          "fn int first() { return 2; }",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 1u, "Function 'first' was already declared"}))
			return;
	}

	TEST(DuplicateVariableDeclarationInSameScopeReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int main() {\n"
		                                                                          "    int value = 1;\n"
		                                                                          "    int value = 2;\n"
		                                                                          "    return value;\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 3u, 5u, "Variable 'value' was already declared"}))
			return;
	}

	TEST(CallToVariableReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int main() {\n"
		                                                                          "    int helper = 1;\n"
		                                                                          "    return helper();\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 3u, 12u, "'helper' is not a function"}))
			return;
	}

	TEST(CallToUndeclaredFunctionReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn int main() { return missing(); }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 24u, "Undeclared function 'missing'"}))
			return;
	}

	TEST(RedundantCastReportsWarning)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "int value = cast(int) 12;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Warning, 1u, 13u, "Redundant cast"}))
			return;
	}

	TEST(InvalidCastReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "string value = cast(string) 12;", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 16u, "Cannot cast from 'int' to 'string'"}))
			return;
	}

	TEST(VariableShadowingEnclosingScopeReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "int count = 1;\n"
		                                                                          "fn int main() {\n"
		                                                                          "    int count = 2;\n"
		                                                                          "    return count;\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 3u, 5u, "Variable 'count' shadows an existing declaration"}))
			return;
	}

	TEST(VariableShadowingGlobalFunctionReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int helper() { return 1; }\n"
		                                                                          "fn int main() { int helper = 12; return helper; }",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 17u, "Variable 'helper' shadows an existing declaration"}))
			return;
	}

	TEST(GlobalVariableCollidingWithFunctionReportsSameScopeDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int answer() { return 1; }\n"
		                                                                          "int answer = 42;",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 1u, "Variable 'answer' was already declared in this scope"}))
			return;
	}

	TEST(CallResultImplicitlyWidensToReturnType)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn char helper() { return 12; }\n"
		                                                                          "fn long main() { return helper(); }",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 2u);

		Declaration* mainDeclaration = GetDeclarationFromTopLevelStatement(&translationUnit, 1);
		ASSERT_EQ(mainDeclaration->type, DECLARATION_TYPE_FUNCTION);
		Statement* returnStatement = GetFunctionBodyStatement(mainDeclaration, 0);
		ASSERT_EQ(returnStatement->type, STATEMENT_TYPE_RETURN);
		if (!AssertCastExpression(returnStatement->returnStmt.expression, BUILTIN_TYPE_LONG))
			return;
		if (!AssertBuiltinType(returnStatement->returnStmt.expression->cast.expression->resolvedType, BUILTIN_TYPE_CHAR))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(UndiscardedNonVoidCallResultReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int helper() { return 1; }\n"
		                                                                          "fn int main() { helper(); return 1; }",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 17u, "Return value of call to 'helper' is unused"}))
			return;
	}

	TEST(DiscardedNonVoidCallAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int helper() { return 1; }\n"
		                                                                          "fn int main() { discard helper(); return 1; }",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(VoidCallWithoutDiscardAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn void helper() {}\n"
		                                                                          "fn int main() { helper(); return 1; }",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(DiscardOnVoidCallReportsRedundantWarning)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn void helper() {}\n"
		                                                                          "fn int main() { discard helper(); return 1; }",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(diag.ErrorCount(), 0u);
		ASSERT_EQ(diag.WarningCount(), 1u);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Warning, 2u, 17u, "Redundant 'discard' on call to 'helper'"}))
			return;
	}

	TEST(CallResultRejectedWhenReturnNarrowsFromLong)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn long helper() { return 12; }\n"
		                                                                          "fn int main() { return helper(); }",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 17u, "return expression of type 'long'"}))
			return;
	}

	TEST(VoidFunctionWithEmptyBodyAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() {}", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(VoidFunctionWithStatementsButNoReturnAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() { int x = 1; }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(AbstractIntegerConstantCannotImplicitlyBindToInt)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "int value = 5000000000;", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 13u, "expected type 'int'"}))
			return;
	}

	TEST(AbstractIntegerCanBeExplicitlyCastToFloat)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "float value = cast(float) 5000000000;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		if (!AssertCastExpression(declaration->variable.initializer, BUILTIN_TYPE_FLOAT))
			return;
		if (!AssertBuiltinType(declaration->variable.initializer->cast.expression->resolvedType, BUILTIN_TYPE_ABSTRACT_INT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TestResults RunSemaTests()
	{
		ResetTestCounters();

		HeapAllocator heap;
		ArenaAllocator arena(&heap, Megabytes(4));

		ScopedTimer timer;

		printf("%sRunning semantic analysis tests...%s\n", TestColor(ANSI_COLOR_BOLD), TestColor(ANSI_COLOR_RESET));

		PrintSection("Constant typing");
		RUN_TEST(IntegerConstantAdoptsHintedFloatType);
		RUN_TEST(IntegerConstantAdoptsHintedDoubleType);
		RUN_TEST(IntegerConstantAdoptsHintedULongType);
		RUN_TEST(SmallIntegerConstantDefaultsToIntWithoutHint);
		RUN_TEST(LargeIntegerConstantDefaultsToAbstractIntegerWithoutHint);
		RUN_TEST(IntegerConstantTooLargeForSignedHintReportsDiagnostic);
		RUN_TEST(IntegerConstantMustFitHintedFloatExactly);

		PrintSection("Returns and conversions");
		RUN_TEST(ReturnStatementImplicitlyCastsToFunctionReturnType);
		RUN_TEST(ReturnStatementTypeMismatchReportsDiagnostic);
		RUN_TEST(VoidFunctionReturningValueReportsDiagnostic);
		RUN_TEST(NonVoidFunctionWithoutReturnReportsDiagnostic);
		RUN_TEST(VoidFunctionWithEmptyBodyAnalyzesSuccessfully);
		RUN_TEST(VoidFunctionWithStatementsButNoReturnAnalyzes);
		RUN_TEST(VariableInitializerIdentifierImplicitlyCasts);
		RUN_TEST(BoolVariableRejectsIntegerLiteralInitializer);
		RUN_TEST(DoubleVariableAdoptsFloatLiteralInitializer);
		RUN_TEST(FloatVariableRejectsDoubleLiteralInitializer);
		RUN_TEST(AbstractIntegerCanBeExplicitlyCastToFloat);
		RUN_TEST(AbstractIntegerConstantCannotImplicitlyBindToInt);
		RUN_TEST(CallResultImplicitlyWidensToReturnType);
		RUN_TEST(CallResultRejectedWhenReturnNarrowsFromLong);
		RUN_TEST(UndiscardedNonVoidCallResultReportsDiagnostic);
		RUN_TEST(DiscardedNonVoidCallAnalyzesSuccessfully);
		RUN_TEST(VoidCallWithoutDiscardAnalyzesSuccessfully);
		RUN_TEST(DiscardOnVoidCallReportsRedundantWarning);

		PrintSection("Name resolution and calls");
		RUN_TEST(UndeclaredIdentifierInInitializerReportsDiagnostic);
		RUN_TEST(SelfReferentialVariableReportsCycleDiagnostic);
		RUN_TEST(DuplicateFunctionDeclarationReportsDiagnostic);
		RUN_TEST(DuplicateVariableDeclarationInSameScopeReportsDiagnostic);
		RUN_TEST(GlobalVariableCollidingWithFunctionReportsSameScopeDiagnostic);
		RUN_TEST(VariableShadowingEnclosingScopeReportsDiagnostic);
		RUN_TEST(VariableShadowingGlobalFunctionReportsDiagnostic);
		RUN_TEST(CallToVariableReportsDiagnostic);
		RUN_TEST(CallToUndeclaredFunctionReportsDiagnostic);
		RUN_TEST(RedundantCastReportsWarning);
		RUN_TEST(InvalidCastReportsDiagnostic);

		f64 totalMs = timer.GetElapsedMilliseconds();
		return PrintTestSummary("Semantic analysis", totalMs);
	}

} // namespace Wandelt
