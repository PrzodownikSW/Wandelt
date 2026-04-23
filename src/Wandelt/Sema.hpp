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
		bool AnalyzeCastExpression(Expression* expr, Type* typeHint);
		bool AnalyzeIncDecExpression(Expression* expr, Type* typeHint);
		bool AnalyzeCallExpression(Expression* expr, Type* typeHint);
		bool AnalyzeAssignmentExpression(Expression* expr, Type* typeHint);
		bool AnalyzeCallArgument(Expression** argumentExpression, Declaration* parameterDecl);

		Expression* InjectCast(Expression* inner, Type* target);

	private:
		SymbolTable m_SymbolTable;
		Allocator* m_DeclAllocator         = nullptr;
		Allocator* m_ExprAllocator         = nullptr;
		TranslationUnit* m_TranslationUnit = nullptr;
		Diagnostics* m_Diagnostics         = nullptr;
		Declaration* m_CurrentFunctionDecl = nullptr;
	};

} // namespace Wandelt
