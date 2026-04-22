#include "Sema.hpp"

namespace Wandelt
{

	Sema::Sema(Allocator* declAllocator, Allocator* exprAllocator, TranslationUnit* tu, Diagnostics* diagnostics)
	    : m_SymbolTable(declAllocator), m_DeclAllocator(declAllocator), m_ExprAllocator(exprAllocator), m_TranslationUnit(tu),
	      m_Diagnostics(diagnostics)
	{
	}

	bool Sema::Analyze()
	{
		m_SymbolTable.PushScope();

		for (i32 i = ANALYSIS_PASS_DECLARATIONS; i < ANALYSIS_PASS_COUNT; ++i)
		{
			if (!AnalyzePass(static_cast<AnalysisPass>(i)))
			{
				m_SymbolTable.PopScope();
				return false;
			}
		}

		m_SymbolTable.PopScope();
		return true;
	}

	bool Sema::AnalyzePass(AnalysisPass pass)
	{
		switch (pass)
		{
		case ANALYSIS_PASS_NONE:
			return true;

		case ANALYSIS_PASS_DECLARATIONS:
			return AnalyzePassDeclarations();

		case ANALYSIS_PASS_DETAILS:
			return AnalyzePassDetails();

		case ANALYSIS_PASS_COUNT:
			ASSERT(false, "Invalid analysis pass: %d", pass);
			return false;

		default:
			ASSERT(false, "Unhandled analysis pass in sema_analyze_pass: %d", pass);
			return false;
		}
	}

	bool Sema::AnalyzePassDeclarations()
	{
		for (Statement* stmt : m_TranslationUnit->statements)
		{
			if (stmt->type != STATEMENT_TYPE_DECLARATION)
				continue;

			Declaration* decl = stmt->declaration.declaration;

			switch (decl->type)
			{
			case DECLARATION_TYPE_PACKAGE:
				break;

			case DECLARATION_TYPE_VARIABLE:
				break; // variables register themselves when encountered during analysis

			case DECLARATION_TYPE_FUNCTION: {
				Symbol* sym = m_SymbolTable.Insert(decl->function.name, SYMBOL_KIND_FUNCTION, decl->function.returnType, decl);
				if (sym == nullptr)
				{
					m_Diagnostics->ReportError(decl->span, m_TranslationUnit->file, "Function '{}' was already declared in this package",
					                           decl->function.name);
					return false;
				}
				break;
			}

			default:
				ASSERT(false, "Unhandled declaration type in AnalyzePassDeclarations: %i", decl->type);
			}
		}

		return true;
	}

	bool Sema::AnalyzePassDetails()
	{
		bool success = true;

		for (Statement* stmt : m_TranslationUnit->statements)
		{
			if (!AnalyzeStatement(stmt))
				success = false;
		}

		return success;
	}

	bool Sema::AnalyzeStatement(Statement* stmt)
	{
		ASSERT(stmt);

		switch (stmt->type)
		{
		case STATEMENT_TYPE_DECLARATION:
			return AnalyzeDeclarationStatement(stmt);
		case STATEMENT_TYPE_EXPRESSION:
			return AnalyzeExpressionStatement(stmt);
		case STATEMENT_TYPE_RETURN:
			return AnalyzeReturnStatement(stmt);
		case STATEMENT_TYPE_BLOCK:
			return AnalyzeBlockStatement(stmt);
		default:
			break;
		}

		ASSERT(false, "Unhandled statement: %i", stmt->type);
		return false;
	}

	bool Sema::AnalyzeDeclarationStatement(Statement* stmt)
	{
		return AnalyzeDeclaration(stmt->declaration.declaration);
	}

	bool Sema::AnalyzeExpressionStatement(Statement* stmt)
	{
		return AnalyzeExpression(stmt->expression.expression, nullptr);
	}

	bool Sema::AnalyzeReturnStatement(Statement* stmt)
	{
		ASSERT(stmt->returnStmt.expression);

		Type* expectedType = m_CurrentFunctionDecl->function.returnType;
		if (!AnalyzeExpression(stmt->returnStmt.expression, expectedType))
			return false;

		if (expectedType == nullptr)
			return true;

		if (stmt->returnStmt.expression->resolvedType != expectedType)
		{
			if (!stmt->returnStmt.expression->resolvedType->IsImplicitlyConvertibleTo(expectedType))
			{
				m_Diagnostics->ReportError(stmt->span, m_TranslationUnit->file,
				                           "Cannot implicitly cast return expression of type '{}' to function return type '{}'",
				                           stmt->returnStmt.expression->resolvedType->ToString(), expectedType->ToString());
				return false;
			}

			stmt->returnStmt.expression = InjectCast(stmt->returnStmt.expression, expectedType);
		}

		return true;
	}

	bool Sema::AnalyzeBlockStatement(Statement* stmt)
	{
		m_SymbolTable.PushScope();
		defer(m_SymbolTable.PopScope());

		bool success = true;
		for (Statement* innerStmt : stmt->block.statements)
		{
			if (!AnalyzeStatement(innerStmt))
				success = false;
		}

		return success;
	}

	bool Sema::AnalyzeDeclaration(Declaration* decl)
	{
		ASSERT(decl);

		switch (decl->resolveStatus)
		{
		case RESOLVE_STATUS_UNRESOLVED: {
			decl->resolveStatus = RESOLVE_STATUS_RESOLVING;
			bool success        = AnalyzeDeclarationInternal(decl);
			decl->resolveStatus = RESOLVE_STATUS_RESOLVED;
			if (!success)
				decl->type = DECLARATION_TYPE_INVALID;
			return success;
		}

		case RESOLVE_STATUS_RESOLVING: {
			m_Diagnostics->ReportError(decl->span, m_TranslationUnit->file, "Cyclic dependency detected while resolving declaration");
			decl->resolveStatus = RESOLVE_STATUS_RESOLVED;
			decl->type          = DECLARATION_TYPE_INVALID;
			return false;
		}

		case RESOLVE_STATUS_RESOLVED:
			return decl->type != DECLARATION_TYPE_INVALID;

		default:
			break;
		}

		ASSERT(false);
		return false;
	}

	bool Sema::AnalyzeDeclarationInternal(Declaration* decl)
	{
		switch (decl->type)
		{
		case DECLARATION_TYPE_INVALID:
			ASSERT(false, "Invalid declaration type!");
			return false;

		case DECLARATION_TYPE_PACKAGE:
			return true;

		case DECLARATION_TYPE_VARIABLE:
			return AnalyzeVariableDeclaration(decl);

		case DECLARATION_TYPE_FUNCTION:
			return AnalyzeFunctionDeclaration(decl);

		case DECLARATION_TYPE_COUNT:
			ASSERT(false, "Invalid declaration type!");
			return false;
		}

		ASSERT(false, "Unhandled declaration: %i", decl->type);
		return false;
	}

	bool Sema::AnalyzeVariableDeclaration(Declaration* decl)
	{
		Symbol* sym = m_SymbolTable.Insert(decl->variable.name, SYMBOL_KIND_VARIABLE, decl->variable.type, decl);
		if (sym == nullptr)
		{
			Symbol* existing = m_SymbolTable.Lookup(decl->variable.name, false);
			ASSERT(existing != nullptr);
			if (existing->scopeDepth == m_SymbolTable.GetScopeDepth())
			{
				m_Diagnostics->ReportError(decl->span, m_TranslationUnit->file, "Variable '{}' was already declared in this scope",
				                           decl->variable.name);
			}
			else
			{
				m_Diagnostics->ReportError(decl->span, m_TranslationUnit->file,
				                           "Variable '{}' shadows an existing declaration from an enclosing scope", decl->variable.name);
			}
			return false;
		}

		ASSERT(decl->variable.initializer);
		if (!AnalyzeExpression(decl->variable.initializer, decl->variable.type))
			return false;

		if (decl->variable.initializer->resolvedType != decl->variable.type)
		{
			if (!decl->variable.initializer->resolvedType->IsImplicitlyConvertibleTo(decl->variable.type))
			{
				m_Diagnostics->ReportError(decl->span, m_TranslationUnit->file,
				                           "Cannot implicitly cast initializer of type '{}' to variable type '{}'",
				                           decl->variable.initializer->resolvedType->ToString(), decl->variable.type->ToString());
				return false;
			}

			decl->variable.initializer = InjectCast(decl->variable.initializer, decl->variable.type);
		}

		return true;
	}

	bool Sema::AnalyzeFunctionDeclaration(Declaration* decl)
	{
		m_CurrentFunctionDecl = decl;

		m_SymbolTable.PushScope();
		defer(m_SymbolTable.PopScope());

		if (!AnalyzeStatement(decl->function.body))
			return false;

		// For now this simple check is enough, TODO: we should eventually check that all code paths return a value if the return type is non-void
		if (decl->function.returnType != Type::GetBuiltinType(BUILTIN_TYPE_VOID))
		{
			ASSERT(decl->function.body->type == STATEMENT_TYPE_BLOCK);

			if (decl->function.body->block.statements.Length() == 0 ||
			    decl->function.body->block.statements[decl->function.body->block.statements.Length() - 1]->type != STATEMENT_TYPE_RETURN)
			{
				m_Diagnostics->ReportError(decl->span, m_TranslationUnit->file,
				                           "Function '{}' with non-void return type must end with a return statement", decl->function.name);
				return false;
			}
		}

		return true;
	}

	bool Sema::AnalyzeExpression(Expression* expr, Type* typeHint)
	{
		ASSERT(expr);

		switch (expr->resolveStatus)
		{
		case RESOLVE_STATUS_UNRESOLVED: {
			expr->resolveStatus = RESOLVE_STATUS_RESOLVING;
			bool success        = AnalyzeExpressionInternal(expr, typeHint);
			expr->resolveStatus = RESOLVE_STATUS_RESOLVED;
			if (!success)
				expr->type = EXPRESSION_TYPE_INVALID;
			return success;
		}

		case RESOLVE_STATUS_RESOLVING: {
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Cyclic dependency detected while resolving expression");
			expr->resolveStatus = RESOLVE_STATUS_RESOLVED;
			expr->type          = EXPRESSION_TYPE_INVALID;
			return false;
		}

		case RESOLVE_STATUS_RESOLVED:
			return true;

		default:
			break;
		}

		ASSERT(false);
	}

	bool Sema::AnalyzeExpressionInternal(Expression* expr, Type* typeHint)
	{
		ASSERT(expr);

		switch (expr->type)
		{
		case EXPRESSION_TYPE_INVALID:
			ASSERT(false, "Invalid expression type!");
			return false;

		case EXPRESSION_TYPE_CONSTANT:
			return AnalyzeConstantExpression(expr, typeHint);

		case EXPRESSION_TYPE_IDENTIFIER:
			return AnalyzeIdentifierExpression(expr, typeHint);

		case EXPRESSION_TYPE_CALL:
			return AnalyzeCallExpression(expr, typeHint);

		case EXPRESSION_TYPE_CAST:
			return AnalyzeCastExpression(expr, typeHint);

		case EXPRESSION_TYPE_COUNT:
			ASSERT(false, "Invalid expression type!");
			return false;
		}

		ASSERT(false, "Unhandled expression type: %i", expr->type);
		return false;
	}

	bool Sema::AnalyzeConstantExpression(Expression* expr, Type* typeHint)
	{
		Type* resolvedType = nullptr;

		if (expr->constant.kind == CONSTANT_KIND_INTEGER)
		{
			if (typeHint && typeHint->CanRepresentIntegerConstant(expr->constant.integerValue))
				resolvedType = typeHint;
			else
				resolvedType = Type::GetDefaultTypeForIntegerConstant(expr->constant.integerValue);
		}
		else if (expr->constant.kind == CONSTANT_KIND_BOOLEAN)
		{
			resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_BOOL);
		}
		else if (expr->constant.kind == CONSTANT_KIND_FLOAT)
		{
			resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_FLOAT);
		}
		else if (expr->constant.kind == CONSTANT_KIND_DOUBLE)
		{
			resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_DOUBLE);
		}
		else
		{
			ASSERT(false, "Unhandled constant kind: %i", expr->constant.kind);
			return false;
		}

		expr->resolvedType = resolvedType;
		ASSERT(expr->resolvedType != nullptr);

		if (typeHint && expr->resolvedType != typeHint)
		{
			if (!expr->resolvedType->IsImplicitlyConvertibleTo(typeHint))
			{
				m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Cannot implicitly cast constant of type '{}' to expected type '{}'",
				                           expr->resolvedType->ToString(), typeHint->ToString());
				return false;
			}

			expr->resolvedType = typeHint;
		}

		return true;
	}

	bool Sema::AnalyzeIdentifierExpression(Expression* expr, Type* /*typeHint*/)
	{
		Symbol* sym = m_SymbolTable.Lookup(expr->identifier.name, true);
		if (sym == nullptr)
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Undeclared identifier '{}'", expr->identifier.name);
			return false;
		}

		if (sym->declarationRef->resolveStatus == RESOLVE_STATUS_RESOLVING)
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Cyclic dependency detected while resolving identifier '{}'",
			                           expr->identifier.name);
			return false;
		}

		if (sym->declarationRef->type == DECLARATION_TYPE_INVALID)
			return false;

		if (sym->declarationRef->resolveStatus == RESOLVE_STATUS_UNRESOLVED)
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file,
			                           "Cannot resolve identifier {} because its details have not been resolved yet", expr->identifier.name);
			return false;
		}

		expr->resolvedType              = sym->type;
		expr->identifier.declarationRef = sym->declarationRef;

		return true;
	}

	bool Sema::AnalyzeCastExpression(Expression* expr, Type* /*typeHint*/)
	{
		Type* target = expr->cast.targetType;
		ASSERT(target != nullptr && target->kind != TYPE_KIND_INVALID);

		// without hint - the cast itself handles conversion
		if (!AnalyzeExpression(expr->cast.expression, nullptr))
			return false;

		Type* source = expr->cast.expression->resolvedType;
		if (source == target)
		{
			m_Diagnostics->ReportWarning(expr->span, m_TranslationUnit->file, "Redundant cast from '{}' to '{}', the type is already '{}'",
			                             source->ToString(), target->ToString(), source->ToString());

			expr->resolvedType = target;
			return true;
		}

		if (source->IsImplicitlyConvertibleTo(target))
		{
			m_Diagnostics->ReportWarning(expr->span, m_TranslationUnit->file, "Unnecessary cast: '{}' is implicitly convertible to '{}'",
			                             source->ToString(), target->ToString());
			expr->resolvedType = target;
			return true;
		}

		if (!source->IsExplicitlyConvertibleTo(target))
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Cannot cast from '{}' to '{}'", source->ToString(), target->ToString());
			return false;
		}

		expr->resolvedType = target;

		return true;
	}

	bool Sema::AnalyzeCallExpression(Expression* expr, Type* /*typeHint*/)
	{
		Symbol* sym = m_SymbolTable.Lookup(expr->call.functionName, true);
		if (sym == nullptr)
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Undeclared function '{}'", expr->call.functionName);
			return false;
		}

		if (sym->kind != SYMBOL_KIND_FUNCTION)
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "'{}' is not a function", expr->call.functionName);
			return false;
		}

		expr->resolvedType        = sym->type;
		expr->call.declarationRef = sym->declarationRef;

		return true;
	}

	Expression* Sema::InjectCast(Expression* inner, Type* target)
	{
		Expression* cast_expr      = m_ExprAllocator->Alloc<Expression>();
		cast_expr->type            = EXPRESSION_TYPE_CAST;
		cast_expr->span            = inner->span;
		cast_expr->cast.targetType = target;
		cast_expr->cast.expression = inner;
		cast_expr->resolvedType    = target;
		cast_expr->resolveStatus   = RESOLVE_STATUS_RESOLVED;

		return cast_expr;
	}

} // namespace Wandelt
