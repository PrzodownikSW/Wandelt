#include "SemaTests.hpp"

#include "TestAstAssertions.hpp"

#include "Wandelt/Parser.hpp"
#include "Wandelt/Sema.hpp"

namespace Wandelt
{

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

	TEST(CharacterConstantAdoptsCharType)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "char value = '\\n';", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_EQ(declaration->variable.initializer->type, EXPRESSION_TYPE_CONSTANT);
		ASSERT_EQ(declaration->variable.initializer->constant.kind, CONSTANT_KIND_CHAR);
		if (!AssertBuiltinType(declaration->variable.initializer->resolvedType, BUILTIN_TYPE_CHAR))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(StringConstantAdoptsStringType)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "string value = \"hello\";", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_EQ(declaration->variable.initializer->type, EXPRESSION_TYPE_CONSTANT);
		ASSERT_EQ(declaration->variable.initializer->constant.kind, CONSTANT_KIND_STRING);
		if (!AssertBuiltinType(declaration->variable.initializer->resolvedType, BUILTIN_TYPE_STRING))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(StringLiteralAdoptsCStringTypeWithHint)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "cstring value = \"hello\";", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);
		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		ASSERT_EQ(declaration->variable.initializer->type, EXPRESSION_TYPE_CONSTANT);
		ASSERT_EQ(declaration->variable.initializer->constant.kind, CONSTANT_KIND_STRING);
		if (!AssertBuiltinType(declaration->variable.initializer->resolvedType, BUILTIN_TYPE_CSTRING))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(NullLiteralAdoptsPointerTypeWithHint)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "int^ ptr = null;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		if (!AssertNullConstant(declaration->variable.initializer))
			return;
		if (!AssertPointerType(declaration->variable.initializer->resolvedType))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
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
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 1u, "must return a value on every control path"}))
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

	TEST(UnaryNegationAdoptsHintedFloatType)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "float value = -12;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		if (!AssertUnaryExpression(declaration->variable.initializer, UNARY_OPERATOR_NEGATE))
			return;
		if (!AssertBuiltinType(declaration->variable.initializer->resolvedType, BUILTIN_TYPE_FLOAT))
			return;
		if (!AssertBuiltinType(declaration->variable.initializer->unary.operand->resolvedType, BUILTIN_TYPE_FLOAT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(UnaryNegationRejectsBooleanOperand)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "bool flag = true; int value = -flag;", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 31u, "requires an arithmetic operand"}))
			return;
	}

	TEST(AddressOfExpressionProducesPointerType)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed = ParseAndAnalyzeSource(alloc, "fn int main() { int value = 1; int^ ptr = &value; return ptr^; }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration      = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		Statement* pointerDeclaration = GetFunctionBodyStatement(declaration, 1);
		ASSERT_EQ(pointerDeclaration->type, STATEMENT_TYPE_DECLARATION);
		if (!AssertUnaryExpression(pointerDeclaration->declaration.declaration->variable.initializer, UNARY_OPERATOR_ADDRESS_OF))
			return;
		if (!AssertPointerType(pointerDeclaration->declaration.declaration->variable.initializer->resolvedType))
			return;

		Statement* returnStatement = GetFunctionBodyStatement(declaration, 2);
		ASSERT_EQ(returnStatement->type, STATEMENT_TYPE_RETURN);
		if (!AssertUnaryExpression(returnStatement->returnStmt.expression, UNARY_OPERATOR_DEREF, true))
			return;
		if (!AssertBuiltinType(returnStatement->returnStmt.expression->resolvedType, BUILTIN_TYPE_INT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(DerefRejectsNonPointerOperand)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn int main() { int value = 1; return value^; }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 39u, "requires a typed pointer operand"}))
			return;
	}

	TEST(BinaryArithmeticExpressionResolvesToCommonType)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "double value = 1 + 2.5f;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		if (!AssertBinaryExpression(declaration->variable.initializer, BINARY_OPERATOR_ADD))
			return;
		if (!AssertBuiltinType(declaration->variable.initializer->resolvedType, BUILTIN_TYPE_DOUBLE))
			return;
		if (!AssertBuiltinType(declaration->variable.initializer->binary.left->resolvedType, BUILTIN_TYPE_DOUBLE))
			return;
		if (!AssertBuiltinType(declaration->variable.initializer->binary.right->resolvedType, BUILTIN_TYPE_DOUBLE))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(BinaryComparisonExpressionResolvesToBool)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "bool value = 1 < 2;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(declaration->type, DECLARATION_TYPE_VARIABLE);
		if (!AssertBinaryExpression(declaration->variable.initializer, BINARY_OPERATOR_LT))
			return;
		if (!AssertBuiltinType(declaration->variable.initializer->resolvedType, BUILTIN_TYPE_BOOL))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(EqualityRejectsMismatchedTypes)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "bool value = true == 1;", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 14u, "requires matching operand types"}))
			return;
	}

	TEST(PointerEqualityWithNullAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn bool is_null(int^ ptr) { return ptr == null; }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration   = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		Statement* returnStatement = GetFunctionBodyStatement(declaration, 0);
		ASSERT_EQ(returnStatement->type, STATEMENT_TYPE_RETURN);
		if (!AssertBinaryExpression(returnStatement->returnStmt.expression, BINARY_OPERATOR_EQ))
			return;
		if (!AssertBuiltinType(returnStatement->returnStmt.expression->resolvedType, BUILTIN_TYPE_BOOL))
			return;
		ASSERT_EQ(returnStatement->returnStmt.expression->binary.right->type, EXPRESSION_TYPE_CAST);
		if (!AssertPointerType(returnStatement->returnStmt.expression->binary.right->cast.targetType))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(GroupExpressionPropagatesInnerType)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn int main() { return (12); }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration   = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		Statement* returnStatement = GetFunctionBodyStatement(declaration, 0);
		ASSERT_EQ(returnStatement->type, STATEMENT_TYPE_RETURN);
		if (!AssertGroupExpression(returnStatement->returnStmt.expression))
			return;
		if (!AssertBuiltinType(returnStatement->returnStmt.expression->resolvedType, BUILTIN_TYPE_INT))
			return;
		if (!AssertBuiltinType(returnStatement->returnStmt.expression->group.inner->resolvedType, BUILTIN_TYPE_INT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(PrefixIncDecExpressionAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed = ParseAndAnalyzeSource(alloc, "fn int main() { int value = 1; ++value; return value; }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration       = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		Statement* expressionStatement = GetFunctionBodyStatement(declaration, 1);
		ASSERT_EQ(expressionStatement->type, STATEMENT_TYPE_EXPRESSION);
		if (!AssertIncDecExpression(expressionStatement->expression.expression, true, false))
			return;
		if (!AssertBuiltinType(expressionStatement->expression.expression->resolvedType, BUILTIN_TYPE_INT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(IncDecRejectsNonVariableOperand)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn int main() { ++12; return 0; }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 17u, "requires a variable operand"}))
			return;
	}

	TEST(AssignmentExpressionAnalyzesAsVoidAndCastsRightHandSide)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "char small = 12;\n"
		                                                                          "fn void main() { int value = 0; value = small; }",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 2u);

		Declaration* declaration       = GetDeclarationFromTopLevelStatement(&translationUnit, 1);
		Statement* expressionStatement = GetFunctionBodyStatement(declaration, 1);
		ASSERT_EQ(expressionStatement->type, STATEMENT_TYPE_EXPRESSION);
		if (!AssertAssignmentExpression(expressionStatement->expression.expression, ASSIGNMENT_OPERATOR_PURE))
			return;
		if (!AssertBuiltinType(expressionStatement->expression.expression->resolvedType, BUILTIN_TYPE_VOID))
			return;
		if (!AssertCastExpression(expressionStatement->expression.expression->assignment.right, BUILTIN_TYPE_INT))
			return;
		if (!AssertBuiltinType(expressionStatement->expression.expression->assignment.right->cast.expression->resolvedType, BUILTIN_TYPE_CHAR))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(AssignmentRejectsNonVariableTarget)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int helper() { return 1; }\n"
		                                                                          "fn void main() { helper() = 1; }",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 18u, "Left-hand side of assignment"}))
			return;
	}

	TEST(DerefAssignmentAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed = ParseAndAnalyzeSource(alloc, "fn void main() { int value = 1; int^ ptr = &value; ptr^ = 3; }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* declaration       = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		Statement* expressionStatement = GetFunctionBodyStatement(declaration, 2);
		ASSERT_EQ(expressionStatement->type, STATEMENT_TYPE_EXPRESSION);
		if (!AssertAssignmentExpression(expressionStatement->expression.expression, ASSIGNMENT_OPERATOR_PURE))
			return;
		if (!AssertUnaryExpression(expressionStatement->expression.expression->assignment.left, UNARY_OPERATOR_DEREF, true))
			return;
		if (!AssertBuiltinType(expressionStatement->expression.expression->assignment.left->resolvedType, BUILTIN_TYPE_INT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(PointerArithmeticReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed = ParseAndAnalyzeSource(alloc, "int value = 1; int^ ptr = &value; int^ other = ptr + 1;", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 48u, "requires arithmetic operands"}))
			return;
	}

	TEST(ChainedAssignmentRejectsAssignmentRightHandSide)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn void main() {\n"
		                                                                          "    int a = 0;\n"
		                                                                          "    int b = 0;\n"
		                                                                          "    int c = 1;\n"
		                                                                          "    a = b = c;\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 5u, 5u, "Cannot implicitly assign value of type 'void' to 'int'"}))
			return;
	}

	TEST(AssignmentExpressionCannotInitializeVariable)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn void main() {\n"
		                                                                          "    int a = 0;\n"
		                                                                          "    int b = 1;\n"
		                                                                          "    int value = a = b;\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(
		        &diag, 0, {Diagnostics::Severity::Error, 4u, 5u, "Cannot implicitly cast initializer of type 'void' to variable type 'int'"}))
			return;
	}

	TEST(AssignmentExpressionCannotBeReturned)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int main() {\n"
		                                                                          "    int a = 0;\n"
		                                                                          "    int b = 1;\n"
		                                                                          "    return a = b;\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(
		        &diag, 0,
		        {Diagnostics::Severity::Error, 4u, 5u, "Cannot implicitly cast return expression of type 'void' to function return type 'int'"}))
			return;
	}

	TEST(AssignmentExpressionCannotBePassedAsCallArgument)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int helper(int x) { return x; }\n"
		                                                                          "fn void main() {\n"
		                                                                          "    int a = 0;\n"
		                                                                          "    int b = 1;\n"
		                                                                          "    discard helper((a = b));\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(
		        &diag, 0, {Diagnostics::Severity::Error, 5u, 20u, "Cannot implicitly cast argument for parameter 'x' from 'void' to 'int'"}))
			return;
	}

	TEST(CompoundAssignmentExpressionAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "char small = 1;\n"
		                                                                          "fn void main() { int value = 0; value += small; }",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 2u);

		Declaration* declaration       = GetDeclarationFromTopLevelStatement(&translationUnit, 1);
		Statement* expressionStatement = GetFunctionBodyStatement(declaration, 1);
		ASSERT_EQ(expressionStatement->type, STATEMENT_TYPE_EXPRESSION);
		if (!AssertAssignmentExpression(expressionStatement->expression.expression, ASSIGNMENT_OPERATOR_ADD))
			return;
		if (!AssertBuiltinType(expressionStatement->expression.expression->resolvedType, BUILTIN_TYPE_VOID))
			return;
		if (!AssertCastExpression(expressionStatement->expression.expression->assignment.right, BUILTIN_TYPE_INT))
			return;
		if (!AssertBuiltinType(expressionStatement->expression.expression->assignment.right->cast.expression->resolvedType, BUILTIN_TYPE_CHAR))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(CompoundAssignmentRejectsNarrowingResult)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() { char value = 1; value += 1000; }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 34u, "cannot be implicitly assigned"}))
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

	TEST(CallToFunctionWithInvalidBodyDoesNotAssert)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int broken(int x, int y) {\n"
		                                                                          "    int res = x + y * false;\n"
		                                                                          "    return res;\n"
		                                                                          "}\n"
		                                                                          "fn int main() { return broken(12, 15); }",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 23u, "constant of type 'bool'"}))
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

	TEST(BareIdentifierExpressionStatementReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn int main() { int x = 12; x; return 1; }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0,
		                              {Diagnostics::Severity::Error, 1u, 29u, "Expression statements without side effects are not allowed"}))
			return;
	}

	TEST(BareBinaryExpressionStatementReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn int main() { int x = 12; x + 1; return 1; }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0,
		                              {Diagnostics::Severity::Error, 1u, 29u, "Expression statements without side effects are not allowed"}))
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

	TEST(FunctionParametersAnalyzeAsLocalBindings)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn int test_1(int x, int y) { return x; }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 1u);

		Declaration* functionDeclaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		ASSERT_EQ(functionDeclaration->type, DECLARATION_TYPE_FUNCTION);
		ASSERT_EQ(functionDeclaration->function.parameters.Length(), 2u);
		ASSERT_STR_EQ(functionDeclaration->function.parameters[0]->variable.name, "x");
		ASSERT_STR_EQ(functionDeclaration->function.parameters[1]->variable.name, "y");
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(PositionalCallArgumentsAnalyzeSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test_1(int x, int y) { return x; }\n"
		                                                                          "fn int main() { return test_1(12, 15); }",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 2u);

		Declaration* mainDeclaration = GetDeclarationFromTopLevelStatement(&translationUnit, 1);
		Statement* returnStatement   = GetFunctionBodyStatement(mainDeclaration, 0);
		if (!AssertCallExpression(returnStatement->returnStmt.expression, "test_1", 2u))
			return;
		if (!AssertBuiltinType(returnStatement->returnStmt.expression->call.arguments[0].expression->resolvedType, BUILTIN_TYPE_INT))
			return;
		if (!AssertBuiltinType(returnStatement->returnStmt.expression->call.arguments[1].expression->resolvedType, BUILTIN_TYPE_INT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(NamedCallArgumentsAnalyzeSuccessfullyAndNormalizeOrder)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test_1(int x, int y) { return x; }\n"
		                                                                          "fn int main() { return test_1(y = 12, x = 2); }",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 2u);

		Declaration* mainDeclaration = GetDeclarationFromTopLevelStatement(&translationUnit, 1);
		Statement* returnStatement   = GetFunctionBodyStatement(mainDeclaration, 0);
		if (!AssertCallExpression(returnStatement->returnStmt.expression, "test_1", 2u))
			return;
		ASSERT_STR_EQ(returnStatement->returnStmt.expression->call.arguments[0].name, "x");
		ASSERT_STR_EQ(returnStatement->returnStmt.expression->call.arguments[1].name, "y");
		ASSERT_EQ(returnStatement->returnStmt.expression->call.arguments[0].expression->constant.integerValue, 2u);
		ASSERT_EQ(returnStatement->returnStmt.expression->call.arguments[1].expression->constant.integerValue, 12u);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(PositionalCallArgumentImplicitlyCastsToParameterType)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test_1(int x, int y) { return x; }\n"
		                                                                          "fn int main() { char small = 12; return test_1(small, 15); }",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 2u);

		Declaration* mainDeclaration = GetDeclarationFromTopLevelStatement(&translationUnit, 1);
		Statement* returnStatement   = GetFunctionBodyStatement(mainDeclaration, 1);
		if (!AssertCallExpression(returnStatement->returnStmt.expression, "test_1", 2u))
			return;
		if (!AssertCastExpression(returnStatement->returnStmt.expression->call.arguments[0].expression, BUILTIN_TYPE_INT))
			return;
		if (!AssertBuiltinType(returnStatement->returnStmt.expression->call.arguments[0].expression->cast.expression->resolvedType,
		                       BUILTIN_TYPE_CHAR))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
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

	TEST(CallWithTooFewPositionalArgumentsReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test_1(int x, int y) { return x; }\n"
		                                                                          "fn int main() { return test_1(12); }",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 24u, "expects 2 arguments, got 1"}))
			return;
	}

	TEST(CallWithUnknownNamedArgumentReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test_1(int x, int y) { return x; }\n"
		                                                                          "fn int main() { return test_1(z = 12, x = 2); }",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 31u, "has no parameter named 'z'"}))
			return;
	}

	TEST(CallWithDuplicateNamedArgumentReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test_1(int x, int y) { return x; }\n"
		                                                                          "fn int main() { return test_1(x = 12, x = 2); }",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 39u, "provided more than once"}))
			return;
	}

	TEST(CallWithMissingNamedArgumentReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test_1(int x, int y) { return x; }\n"
		                                                                          "fn int main() { return test_1(x = 12); }",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 24u, "Missing named argument for parameter 'y'"}))
			return;
	}

	TEST(IfStatementWithBoolConditionAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() { if true { } }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(IfStatementWithBoolVariableConditionAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() { bool cond = true; if cond { } }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(IfStatementWithIntegerConditionReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() { if 1 { } }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 21u, "condition must be of type 'bool'"}))
			return;
	}

	TEST(IfStatementAnalyzesThenAndElseBranches)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn void main() {\n"
		                                                                          "    if true { undef_a; } else { undef_b; }\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_TRUE(diag.CapturedCount() >= 2u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 15u, "undef_a"}))
			return;
		if (!AssertCapturedDiagnostic(&diag, 1, {Diagnostics::Severity::Error, 2u, 33u, "undef_b"}))
			return;
	}

	TEST(IfStatementElseIfChainAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed = ParseAndAnalyzeSource(alloc, "fn void main() { if true { } else if 2 == 4 { } else { } }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(IfStatementBranchVariableDoesNotLeakOutOfIf)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int main() {\n"
		                                                                          "    if true { int inside = 1; }\n"
		                                                                          "    return inside;\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_TRUE(diag.CapturedCount() >= 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 3u, 12u, "inside"}))
			return;
	}

	TEST(NonVoidFunctionReturningOnlyInsideIfWithoutElseReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test() {\n"
		                                                                          "    if true { return 1; }\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 1u, "must return a value on every control path"}))
			return;
	}

	TEST(NonVoidFunctionReturningInBothIfAndElseAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test() {\n"
		                                                                          "    if true { return 1; } else { return 2; }\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(NonVoidFunctionReturningInEveryBranchOfIfElseIfElseAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test() {\n"
		                                                                          "    if true { return 1; } else if 2 == 4 { return 2; } else { return 3; }\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(NonVoidFunctionMissingReturnInTerminalElseReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test() {\n"
		                                                                          "    if true { return 1; } else if 2 == 4 { return 2; } else { }\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 1u, "must return a value on every control path"}))
			return;
	}

	TEST(NonVoidFunctionWithTrailingReturnAfterPartialIfAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test() {\n"
		                                                                          "    if true { return 1; }\n"
		                                                                          "    return 2;\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(WhileStatementWithBoolConditionAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() { while true { } }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(WhileStatementWithBoolVariableConditionAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed = ParseAndAnalyzeSource(alloc, "fn void main() { bool cond = true; while cond { } }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(WhileStatementWithIntegerConditionReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() { while 1 { } }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 24u, "condition must be of type 'bool'"}))
			return;
	}

	TEST(WhileStatementAnalyzesBody)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn void main() {\n"
		                                                                          "    while true { undef_body; }\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_TRUE(diag.CapturedCount() >= 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 18u, "undef_body"}))
			return;
	}

	TEST(WhileStatementBodyVariableDoesNotLeakOut)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int main() {\n"
		                                                                          "    while true { int inside = 1; }\n"
		                                                                          "    return inside;\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_TRUE(diag.CapturedCount() >= 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 3u, 12u, "inside"}))
			return;
	}

	TEST(BreakStatementInsideWhileAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() { while true { break; } }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(ContinueStatementInsideWhileAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() { while true { continue; } }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(BreakStatementOutsideLoopReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() { break; }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 18u, "'break' statement is only allowed inside a loop"}))
			return;
	}

	TEST(ContinueStatementOutsideLoopReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() { continue; }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 18u, "'continue' statement is only allowed inside a loop"}))
			return;
	}

	TEST(BreakInsideIfInsideWhileAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed = ParseAndAnalyzeSource(alloc, "fn void main() { while true { if true { break; } } }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(BreakInsideNestedWhileAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn void main() {\n"
		                                                                          "    while true { while false { break; } break; }\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(BreakAfterNestedWhileEndsOutsideInnerLoop)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn void main() {\n"
		                                                                          "    while true { }\n"
		                                                                          "    break;\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 3u, 5u, "'break' statement is only allowed inside a loop"}))
			return;
	}

	TEST(NonVoidFunctionWithWhileButNoReturnReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test() {\n"
		                                                                          "    while true { return 1; }\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 1u, "must return a value on every control path"}))
			return;
	}

	TEST(ForStatementWithBoolConditionAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() { for int x = 0; x < 10; x++ { } }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(ForStatementWithIntegerConditionReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn void main() { for int x = 0; 1; x++ { } }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 33u, "condition must be of type 'bool'"}))
			return;
	}

	TEST(ForStatementIncrementWithoutSideEffectReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed = ParseAndAnalyzeSource(alloc, "fn void main() { for int x = 0; x < 10; x + 1 { } }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 41u, "increment expression has no effect"}))
			return;
	}

	TEST(ForStatementInitVariableVisibleInConditionAndIncrementAndBody)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn void main() {\n"
		                                                                          "    for int x = 0; x < 10; x++ { x++; }\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 0u);
	}

	TEST(ForStatementInitVariableDoesNotLeakOut)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int main() {\n"
		                                                                          "    for int x = 0; x < 10; x++ { }\n"
		                                                                          "    return x;\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_TRUE(diag.CapturedCount() >= 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 3u, 12u, "x"}))
			return;
	}

	TEST(BreakInsideForAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn void main() {\n"
		                                                                          "    for int x = 0; x < 10; x++ {\n"
		                                                                          "        if x == 5 { break; }\n"
		                                                                          "    }\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(ContinueInsideForAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed = ParseAndAnalyzeSource(alloc, "fn void main() { for int x = 0; x < 10; x++ { continue; } }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(ForStatementWithExpressionInitAnalyzes)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed = ParseAndAnalyzeSource(alloc, "fn void main() { int x = 0; for x = 0; x < 10; x++ { } }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(NonVoidFunctionWithForButNoReturnReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int test() {\n"
		                                                                          "    for int x = 0; x < 10; x++ { return x; }\n"
		                                                                          "}",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 1u, "must return a value on every control path"}))
			return;
	}

	TEST(ArrayLiteralInitializerAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "int[4] arr = [1, 2, 3, 4];", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		Type* elementType        = nullptr;
		if (!AssertArrayType(declaration->variable.type, 4u, &elementType))
			return;
		if (!AssertBuiltinType(elementType, BUILTIN_TYPE_INT))
			return;
		if (!AssertArrayLiteralExpression(declaration->variable.initializer, 4u))
			return;
		if (!AssertBuiltinType(declaration->variable.initializer->resolvedType->array.elementType, BUILTIN_TYPE_INT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(ArrayImplicitlyBorrowsToSliceVariable)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "int[4] arr = [1, 2, 3, 4]; int[] view = arr;", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_EQ(translationUnit.statements.Length(), 2u);

		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 1);
		Type* targetType         = nullptr;
		if (!AssertCastExpression(declaration->variable.initializer, &targetType))
			return;
		Type* elementType = nullptr;
		if (!AssertSliceType(targetType, &elementType))
			return;
		if (!AssertBuiltinType(elementType, BUILTIN_TYPE_INT))
			return;
		if (!AssertArrayType(declaration->variable.initializer->cast.expression->resolvedType, 4u, &elementType))
			return;
		if (!AssertBuiltinType(elementType, BUILTIN_TYPE_INT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(ArrayLiteralFillSyntaxAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "int[10] x = [1, 2, 3...];", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		if (!AssertArrayLiteralExpression(declaration->variable.initializer, 3u, true))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(MultidimensionalArrayFillAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "int[3][4] m = [[1, 3, 0...]...];", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		if (!AssertArrayLiteralExpression(declaration->variable.initializer, 1u, true))
			return;
		if (!AssertArrayLiteralExpression(declaration->variable.initializer->arrayLiteral.items[0], 3u, true))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(ArrayParameterAndReturnAnalyzeSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn int[4] id(int[4] value) { return value; }\n"
		                                                                          "int[4] arr = [1, 2, 3, 4];\n"
		                                                                          "int[4] other = id(arr);",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(SliceParameterAcceptsArrayArgument)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn sz count(int[] values) { return $len(values); }\n"
		                                                                          "int[4] arr = [1, 2, 3, 4];\n"
		                                                                          "sz size = count(arr);",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		Declaration* declaration = GetDeclarationFromTopLevelStatement(&translationUnit, 2);
		if (!AssertCallExpression(declaration->variable.initializer, "count", 1u))
			return;
		Type* targetType = nullptr;
		if (!AssertCastExpression(declaration->variable.initializer->call.arguments[0].expression, &targetType))
			return;
		Type* elementType = nullptr;
		if (!AssertSliceType(targetType, &elementType))
			return;
		if (!AssertBuiltinType(elementType, BUILTIN_TYPE_INT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(AsymmetricMultidimensionalArrayAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed = ParseAndAnalyzeSource(alloc, "fn int main() { int[2][4] arr = [[1, 2, 3, 4], [5, 6, 7, 8]]; return arr[1][2]; }", &diag,
		                                      &translationUnit);

		ASSERT_TRUE(analyzed);
		Declaration* declaration   = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		Statement* returnStatement = GetFunctionBodyStatement(declaration, 1);
		ASSERT_EQ(returnStatement->type, STATEMENT_TYPE_RETURN);
		if (!AssertIndexExpression(returnStatement->returnStmt.expression))
			return;
		if (!AssertBuiltinType(returnStatement->returnStmt.expression->resolvedType, BUILTIN_TYPE_INT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(IndexExpressionAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed = ParseAndAnalyzeSource(alloc, "fn int main() { int[4] arr = [1, 2, 3, 4]; return arr[0]; }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		Declaration* declaration   = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		Statement* returnStatement = GetFunctionBodyStatement(declaration, 1);
		if (!AssertIndexExpression(returnStatement->returnStmt.expression))
			return;
		if (!AssertBuiltinType(returnStatement->returnStmt.expression->resolvedType, BUILTIN_TYPE_INT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(SliceIndexExpressionAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed =
		    ParseAndAnalyzeSource(alloc, "fn int first(int[] values) { return values[0]; } int[4] arr = [1, 2, 3, 4];", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		Declaration* declaration   = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		Statement* returnStatement = GetFunctionBodyStatement(declaration, 0);
		if (!AssertIndexExpression(returnStatement->returnStmt.expression))
			return;
		if (!AssertBuiltinType(returnStatement->returnStmt.expression->resolvedType, BUILTIN_TYPE_INT))
			return;
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(LenIntrinsicAnalyzesForArrayAndSlice)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "fn sz a(int[] values) { return $len(values); }\n"
		                                                                          "fn sz b() { int[4] arr = [1, 2, 3, 4]; return $len(arr); }",
		                                                        &diag, &translationUnit);

		ASSERT_TRUE(analyzed);

		Declaration* sliceFunction = GetDeclarationFromTopLevelStatement(&translationUnit, 0);
		Statement* sliceReturn     = GetFunctionBodyStatement(sliceFunction, 0);
		Declaration* arrayFunction = GetDeclarationFromTopLevelStatement(&translationUnit, 1);
		Statement* arrayReturn     = GetFunctionBodyStatement(arrayFunction, 1);

		if (!AssertIntrinsicExpression(sliceReturn->returnStmt.expression, INTRINSIC_KIND_LEN, 1u))
			return;
		if (!AssertBuiltinType(sliceReturn->returnStmt.expression->resolvedType, BUILTIN_TYPE_SZ))
			return;
		if (!AssertIntrinsicExpression(arrayReturn->returnStmt.expression, INTRINSIC_KIND_LEN, 1u))
			return;
		if (!AssertBuiltinType(arrayReturn->returnStmt.expression->resolvedType, BUILTIN_TYPE_SZ))
			return;

		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(IndexAssignmentAnalyzesSuccessfully)
	{
		Diagnostics diag;
		TranslationUnit translationUnit = {};
		bool analyzed = ParseAndAnalyzeSource(alloc, "fn void main() { int[4] arr = [1, 2, 3, 4]; arr[0] = 9; }", &diag, &translationUnit);

		ASSERT_TRUE(analyzed);
		ASSERT_NO_DIAGNOSTICS(diag);
	}

	TEST(ConstantOutOfBoundsArrayIndexReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc,
		                                                        "int[4] arr = [1, 2, 3, 4];\n"
		                                                                          "int _xxx = arr[4];",
		                                                        &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 2u, 16u, "Array index 4 is out of bounds"}))
			return;
	}

	TEST(ArrayLiteralWrongLengthReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "int[4] arr = [1, 2, 3];", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 14u, "must initialize exactly 4 elements"}))
			return;
	}

	TEST(ArrayLiteralWithoutArrayContextReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "int value = [1, 2, 3];", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 13u, "Array literal requires an expected array type"}))
			return;
	}

	TEST(IndexingNonArrayReportsDiagnostic)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "fn int main() { int value = 1; return value[0]; }", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 39u, "requires an array or slice operand"}))
			return;
	}

	TEST(LenIntrinsicRejectsNonArrayOrSliceArgument)
	{
		Diagnostics diag;
		Diagnostics::CaptureScope capture(diag);
		TranslationUnit translationUnit = {};
		bool analyzed                   = ParseAndAnalyzeSource(alloc, "usz value = $len(12);", &diag, &translationUnit);

		ASSERT_FALSE(analyzed);
		ASSERT_EQ(diag.CapturedCount(), 1u);
		if (!AssertCapturedDiagnostic(&diag, 0, {Diagnostics::Severity::Error, 1u, 18u, "requires an array or slice argument"}))
			return;
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
		RUN_TEST(CharacterConstantAdoptsCharType);
		RUN_TEST(StringConstantAdoptsStringType);
		RUN_TEST(StringLiteralAdoptsCStringTypeWithHint);
		RUN_TEST(NullLiteralAdoptsPointerTypeWithHint);

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
		RUN_TEST(UnaryNegationAdoptsHintedFloatType);
		RUN_TEST(UnaryNegationRejectsBooleanOperand);
		RUN_TEST(AddressOfExpressionProducesPointerType);
		RUN_TEST(DerefRejectsNonPointerOperand);
		RUN_TEST(BinaryArithmeticExpressionResolvesToCommonType);
		RUN_TEST(BinaryComparisonExpressionResolvesToBool);
		RUN_TEST(EqualityRejectsMismatchedTypes);
		RUN_TEST(PointerEqualityWithNullAnalyzesSuccessfully);
		RUN_TEST(GroupExpressionPropagatesInnerType);
		RUN_TEST(PrefixIncDecExpressionAnalyzesSuccessfully);
		RUN_TEST(IncDecRejectsNonVariableOperand);
		RUN_TEST(AssignmentExpressionAnalyzesAsVoidAndCastsRightHandSide);
		RUN_TEST(AssignmentRejectsNonVariableTarget);
		RUN_TEST(DerefAssignmentAnalyzesSuccessfully);
		RUN_TEST(ChainedAssignmentRejectsAssignmentRightHandSide);
		RUN_TEST(AssignmentExpressionCannotInitializeVariable);
		RUN_TEST(AssignmentExpressionCannotBeReturned);
		RUN_TEST(AssignmentExpressionCannotBePassedAsCallArgument);
		RUN_TEST(CompoundAssignmentExpressionAnalyzesSuccessfully);
		RUN_TEST(PointerArithmeticReportsDiagnostic);
		RUN_TEST(CompoundAssignmentRejectsNarrowingResult);
		RUN_TEST(CallResultImplicitlyWidensToReturnType);
		RUN_TEST(CallResultRejectedWhenReturnNarrowsFromLong);
		RUN_TEST(FunctionParametersAnalyzeAsLocalBindings);
		RUN_TEST(PositionalCallArgumentsAnalyzeSuccessfully);
		RUN_TEST(NamedCallArgumentsAnalyzeSuccessfullyAndNormalizeOrder);
		RUN_TEST(PositionalCallArgumentImplicitlyCastsToParameterType);
		RUN_TEST(UndiscardedNonVoidCallResultReportsDiagnostic);
		RUN_TEST(DiscardedNonVoidCallAnalyzesSuccessfully);
		RUN_TEST(VoidCallWithoutDiscardAnalyzesSuccessfully);
		RUN_TEST(DiscardOnVoidCallReportsRedundantWarning);
		RUN_TEST(BareIdentifierExpressionStatementReportsDiagnostic);
		RUN_TEST(BareBinaryExpressionStatementReportsDiagnostic);
		RUN_TEST(ArrayLiteralInitializerAnalyzesSuccessfully);
		RUN_TEST(ArrayImplicitlyBorrowsToSliceVariable);
		RUN_TEST(ArrayLiteralFillSyntaxAnalyzesSuccessfully);
		RUN_TEST(MultidimensionalArrayFillAnalyzesSuccessfully);
		RUN_TEST(ArrayParameterAndReturnAnalyzeSuccessfully);
		RUN_TEST(SliceParameterAcceptsArrayArgument);
		RUN_TEST(AsymmetricMultidimensionalArrayAnalyzesSuccessfully);
		RUN_TEST(IndexExpressionAnalyzesSuccessfully);
		RUN_TEST(SliceIndexExpressionAnalyzesSuccessfully);
		RUN_TEST(IndexAssignmentAnalyzesSuccessfully);
		RUN_TEST(LenIntrinsicAnalyzesForArrayAndSlice);
		RUN_TEST(ConstantOutOfBoundsArrayIndexReportsDiagnostic);
		RUN_TEST(ArrayLiteralWrongLengthReportsDiagnostic);
		RUN_TEST(ArrayLiteralWithoutArrayContextReportsDiagnostic);
		RUN_TEST(IndexingNonArrayReportsDiagnostic);
		RUN_TEST(LenIntrinsicRejectsNonArrayOrSliceArgument);

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
		RUN_TEST(CallToFunctionWithInvalidBodyDoesNotAssert);
		RUN_TEST(CallWithTooFewPositionalArgumentsReportsDiagnostic);
		RUN_TEST(CallWithUnknownNamedArgumentReportsDiagnostic);
		RUN_TEST(CallWithDuplicateNamedArgumentReportsDiagnostic);
		RUN_TEST(CallWithMissingNamedArgumentReportsDiagnostic);
		RUN_TEST(RedundantCastReportsWarning);
		RUN_TEST(InvalidCastReportsDiagnostic);

		PrintSection("Control flow");
		RUN_TEST(IfStatementWithBoolConditionAnalyzes);
		RUN_TEST(IfStatementWithBoolVariableConditionAnalyzes);
		RUN_TEST(IfStatementWithIntegerConditionReportsDiagnostic);
		RUN_TEST(IfStatementAnalyzesThenAndElseBranches);
		RUN_TEST(IfStatementElseIfChainAnalyzes);
		RUN_TEST(IfStatementBranchVariableDoesNotLeakOutOfIf);
		RUN_TEST(NonVoidFunctionReturningOnlyInsideIfWithoutElseReportsDiagnostic);
		RUN_TEST(NonVoidFunctionReturningInBothIfAndElseAnalyzes);
		RUN_TEST(NonVoidFunctionReturningInEveryBranchOfIfElseIfElseAnalyzes);
		RUN_TEST(NonVoidFunctionMissingReturnInTerminalElseReportsDiagnostic);
		RUN_TEST(NonVoidFunctionWithTrailingReturnAfterPartialIfAnalyzes);
		RUN_TEST(WhileStatementWithBoolConditionAnalyzes);
		RUN_TEST(WhileStatementWithBoolVariableConditionAnalyzes);
		RUN_TEST(WhileStatementWithIntegerConditionReportsDiagnostic);
		RUN_TEST(WhileStatementAnalyzesBody);
		RUN_TEST(WhileStatementBodyVariableDoesNotLeakOut);
		RUN_TEST(BreakStatementInsideWhileAnalyzes);
		RUN_TEST(ContinueStatementInsideWhileAnalyzes);
		RUN_TEST(BreakStatementOutsideLoopReportsDiagnostic);
		RUN_TEST(ContinueStatementOutsideLoopReportsDiagnostic);
		RUN_TEST(BreakInsideIfInsideWhileAnalyzes);
		RUN_TEST(BreakInsideNestedWhileAnalyzes);
		RUN_TEST(BreakAfterNestedWhileEndsOutsideInnerLoop);
		RUN_TEST(NonVoidFunctionWithWhileButNoReturnReportsDiagnostic);
		RUN_TEST(ForStatementWithBoolConditionAnalyzes);
		RUN_TEST(ForStatementWithIntegerConditionReportsDiagnostic);
		RUN_TEST(ForStatementIncrementWithoutSideEffectReportsDiagnostic);
		RUN_TEST(ForStatementInitVariableVisibleInConditionAndIncrementAndBody);
		RUN_TEST(ForStatementInitVariableDoesNotLeakOut);
		RUN_TEST(BreakInsideForAnalyzes);
		RUN_TEST(ContinueInsideForAnalyzes);
		RUN_TEST(ForStatementWithExpressionInitAnalyzes);
		RUN_TEST(NonVoidFunctionWithForButNoReturnReportsDiagnostic);

		f64 totalMs = timer.GetElapsedMilliseconds();
		return PrintTestSummary("Semantic analysis", totalMs);
	}

} // namespace Wandelt
