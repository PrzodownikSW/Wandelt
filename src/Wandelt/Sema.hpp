#pragma once

#include "Wandelt/Defines.hpp"
#include "Wandelt/Memory.hpp"
#include "Wandelt/Parser.hpp"
#include "Wandelt/SymbolTable.hpp"

namespace Wandelt
{

	enum AnalysisPass
	{
		ANALYSIS_PASS_NONE,

		ANALYSIS_PASS_DECLARATIONS,
		ANALYSIS_PASS_DETAILS,

		ANALYSIS_PASS_COUNT
	};

	class Sema
	{
	public:
		Sema(Allocator* declAllocator, Allocator* exprAllocator, TranslationUnit* tu, Diagnostics* diagnostics);
		~Sema() = default;

		NONCOPYABLE(Sema);
		NONMOVABLE(Sema);

	public:
		bool Analyze();

	private:
		bool AnalyzePass(AnalysisPass pass);
		bool AnalyzePassDeclarations();
		bool AnalyzePassDetails();

	private:
		bool AnalyzeStatement(Statement* stmt);
		bool AnalyzeDeclarationStatement(Statement* stmt);
		bool AnalyzeExpressionStatement(Statement* stmt);
		bool AnalyzeReturnStatement(Statement* stmt);
		bool AnalyzeBlockStatement(Statement* stmt);
		bool AnalyzeIfStatement(Statement* stmt);
		bool AnalyzeWhileStatement(Statement* stmt);
		bool AnalyzeForStatement(Statement* stmt);
		bool AnalyzeBreakStatement(Statement* stmt);
		bool AnalyzeContinueStatement(Statement* stmt);

		bool AnalyzeDeclaration(Declaration* decl);
		bool AnalyzeDeclarationInternal(Declaration* decl);
		bool AnalyzeVariableDeclaration(Declaration* decl);
		bool AnalyzeFunctionDeclaration(Declaration* decl);

		bool AnalyzeExpression(Expression* expr, Type* typeHint);
		bool AnalyzeExpressionInternal(Expression* expr, Type* typeHint);
		bool AnalyzeConstantExpression(Expression* expr, Type* typeHint);
		bool AnalyzeUnaryExpression(Expression* expr, Type* typeHint);
		bool AnalyzeBinaryExpression(Expression* expr, Type* typeHint);
		bool AnalyzeGroupExpression(Expression* expr, Type* typeHint);
		bool AnalyzeIdentifierExpression(Expression* expr, Type* typeHint);
		bool AnalyzeIntrinsicExpression(Expression* expr, Type* typeHint);
		bool AnalyzeCastExpression(Expression* expr, Type* typeHint);
		bool AnalyzeIncDecExpression(Expression* expr, Type* typeHint);
		bool AnalyzeCallExpression(Expression* expr, Type* typeHint);
		bool AnalyzeAssignmentExpression(Expression* expr, Type* typeHint);
		bool AnalyzeArrayLiteralExpression(Expression* expr, Type* typeHint);
		bool AnalyzeIndexExpression(Expression* expr, Type* typeHint);
		bool AnalyzeCallArgument(Expression** argumentExpression, Declaration* parameterDecl);

		bool ExpressionHasSideEffect(const Expression* expr);
		bool StatementAlwaysReturns(const Statement* stmt);

		Expression* InjectCast(Expression* inner, Type* target);

	private:
		SymbolTable m_SymbolTable;
		Allocator* m_DeclAllocator         = nullptr;
		Allocator* m_ExprAllocator         = nullptr;
		TranslationUnit* m_TranslationUnit = nullptr;
		Diagnostics* m_Diagnostics         = nullptr;
		Declaration* m_CurrentFunctionDecl = nullptr;
		i32 m_LoopDepth                    = 0;
	};

} // namespace Wandelt
