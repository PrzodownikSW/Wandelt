#include "Sema.hpp"

namespace Wandelt
{
	static bool IsVariableLValue(Expression* expr)
	{
		return expr != nullptr && expr->type == EXPRESSION_TYPE_IDENTIFIER && expr->identifier.declarationRef != nullptr &&
		       expr->identifier.declarationRef->type == DECLARATION_TYPE_VARIABLE;
	}

	static bool TryResolveAbstractIntegerConstantToType(Expression* expr, Type* targetType)
	{
		if (expr == nullptr || targetType == nullptr)
			return false;

		if (expr->type != EXPRESSION_TYPE_CONSTANT || expr->constant.kind != CONSTANT_KIND_INTEGER)
			return false;

		if (expr->resolvedType != Type::GetBuiltinType(BUILTIN_TYPE_ABSTRACT_INT))
			return false;

		if (!targetType->CanRepresentIntegerConstant(expr->constant.integerValue))
			return false;

		expr->resolvedType = targetType;
		return true;
	}

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
		Expression* expr = stmt->expression.expression;
		if (!AnalyzeExpression(expr, nullptr))
			return false;

		const bool discarded   = stmt->expression.discarded;
		const bool isCall      = expr->type == EXPRESSION_TYPE_CALL;
		const bool returnsVoid = isCall && expr->resolvedType == Type::GetBuiltinType(BUILTIN_TYPE_VOID);

		if (!discarded && isCall && !returnsVoid)
		{
			m_Diagnostics->ReportError(stmt->span, m_TranslationUnit->file,
			                           "Return value of call to '{}' is unused; use 'discard' to ignore it explicitly", expr->call.functionName);
			return false;
		}

		if (discarded && isCall && returnsVoid)
		{
			m_Diagnostics->ReportWarning(stmt->span, m_TranslationUnit->file, "Redundant 'discard' on call to '{}' which returns 'void'",
			                             expr->call.functionName);
		}

		return true;
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

		for (Declaration* parameterDecl : decl->function.parameters)
		{
			ASSERT(parameterDecl != nullptr);
			ASSERT(parameterDecl->type == DECLARATION_TYPE_VARIABLE);

			parameterDecl->resolveStatus = RESOLVE_STATUS_RESOLVED;

			Symbol* sym = m_SymbolTable.Insert(parameterDecl->variable.name, SYMBOL_KIND_VARIABLE, parameterDecl->variable.type, parameterDecl);
			if (sym == nullptr)
			{
				Symbol* existing = m_SymbolTable.Lookup(parameterDecl->variable.name, false);
				ASSERT(existing != nullptr);
				if (existing->scopeDepth == m_SymbolTable.GetScopeDepth())
				{
					m_Diagnostics->ReportError(parameterDecl->span, m_TranslationUnit->file, "Variable '{}' was already declared in this scope",
					                           parameterDecl->variable.name);
				}
				else
				{
					m_Diagnostics->ReportError(parameterDecl->span, m_TranslationUnit->file,
					                           "Variable '{}' shadows an existing declaration from an enclosing scope", parameterDecl->variable.name);
				}

				return false;
			}
		}

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

		case EXPRESSION_TYPE_UNARY:
			return AnalyzeUnaryExpression(expr, typeHint);

		case EXPRESSION_TYPE_BINARY:
			return AnalyzeBinaryExpression(expr, typeHint);

		case EXPRESSION_TYPE_GROUP:
			return AnalyzeGroupExpression(expr, typeHint);

		case EXPRESSION_TYPE_IDENTIFIER:
			return AnalyzeIdentifierExpression(expr, typeHint);

		case EXPRESSION_TYPE_CALL:
			return AnalyzeCallExpression(expr, typeHint);

		case EXPRESSION_TYPE_CAST:
			return AnalyzeCastExpression(expr, typeHint);

		case EXPRESSION_TYPE_INCDEC:
			return AnalyzeIncDecExpression(expr, typeHint);

		case EXPRESSION_TYPE_ASSIGNMENT:
			return AnalyzeAssignmentExpression(expr, typeHint);

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
		else if (expr->constant.kind == CONSTANT_KIND_CHAR)
		{
			resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_CHAR);
		}
		else if (expr->constant.kind == CONSTANT_KIND_STRING)
		{
			if (typeHint == Type::GetBuiltinType(BUILTIN_TYPE_CSTRING))
				resolvedType = typeHint;
			else
				resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_STRING);
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

	bool Sema::AnalyzeUnaryExpression(Expression* expr, Type* typeHint)
	{
		Type* operandHint = nullptr;
		if (typeHint != nullptr && typeHint->IsArithmetic())
			operandHint = typeHint;

		if (!AnalyzeExpression(expr->unary.operand, operandHint))
			return false;

		Type* operandType = expr->unary.operand->resolvedType;
		ASSERT(operandType != nullptr);

		if (!operandType->IsArithmetic())
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Unary operator '{}' requires an arithmetic operand, got '{}'",
			                           UnaryOperatorToTokenCStr(expr->unary.op), operandType->ToString());
			return false;
		}

		expr->resolvedType = operandType;
		return true;
	}

	bool Sema::AnalyzeBinaryExpression(Expression* expr, Type* typeHint)
	{
		Type* operandHint = nullptr;
		if (!IsBinaryOpAComparison(expr->binary.op) && typeHint != nullptr && typeHint->IsArithmetic())
			operandHint = typeHint;

		if (!AnalyzeExpression(expr->binary.left, operandHint))
			return false;

		Type* rightHint = operandHint;
		if (rightHint == nullptr && expr->binary.left->resolvedType != nullptr && expr->binary.left->resolvedType->IsArithmetic() &&
		    expr->binary.left->resolvedType != Type::GetBuiltinType(BUILTIN_TYPE_ABSTRACT_INT))
		{
			rightHint = expr->binary.left->resolvedType;
		}

		if (!AnalyzeExpression(expr->binary.right, rightHint))
			return false;

		Type* leftType  = expr->binary.left->resolvedType;
		Type* rightType = expr->binary.right->resolvedType;
		ASSERT(leftType != nullptr);
		ASSERT(rightType != nullptr);

		if (leftType == Type::GetBuiltinType(BUILTIN_TYPE_ABSTRACT_INT) && rightType->IsArithmetic())
			TryResolveAbstractIntegerConstantToType(expr->binary.left, rightType);

		if (rightType == Type::GetBuiltinType(BUILTIN_TYPE_ABSTRACT_INT) && leftType->IsArithmetic())
			TryResolveAbstractIntegerConstantToType(expr->binary.right, leftType);

		leftType  = expr->binary.left->resolvedType;
		rightType = expr->binary.right->resolvedType;

		if (IsBinaryOpRelationalComparison(expr->binary.op))
		{
			if (!leftType->IsArithmetic() || !rightType->IsArithmetic())
			{
				m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file,
				                           "Relational operator '{}' requires arithmetic operands, got '{}' and '{}'",
				                           BinaryOperatorToTokenCStr(expr->binary.op), leftType->ToString(), rightType->ToString());
				return false;
			}

			Type* commonType = Type::GetImplicitCommonType(leftType, rightType);
			if (commonType == nullptr)
			{
				m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Operator '{}' cannot be applied to operands of type '{}' and '{}'",
				                           BinaryOperatorToTokenCStr(expr->binary.op), leftType->ToString(), rightType->ToString());
				return false;
			}

			if (expr->binary.left->resolvedType != commonType)
				expr->binary.left = InjectCast(expr->binary.left, commonType);

			if (expr->binary.right->resolvedType != commonType)
				expr->binary.right = InjectCast(expr->binary.right, commonType);

			expr->resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_BOOL);
			return true;
		}

		if (IsBinaryOpEqualityComparison(expr->binary.op))
		{
			if (leftType == rightType)
			{
				expr->resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_BOOL);
				return true;
			}

			if (!leftType->IsArithmetic() || !rightType->IsArithmetic())
			{
				m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file,
				                           "Equality operator '{}' requires matching operand types, got '{}' and '{}'",
				                           BinaryOperatorToTokenCStr(expr->binary.op), leftType->ToString(), rightType->ToString());
				return false;
			}

			Type* commonType = Type::GetImplicitCommonType(leftType, rightType);
			if (commonType == nullptr)
			{
				m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Operator '{}' cannot be applied to operands of type '{}' and '{}'",
				                           BinaryOperatorToTokenCStr(expr->binary.op), leftType->ToString(), rightType->ToString());
				return false;
			}

			if (expr->binary.left->resolvedType != commonType)
				expr->binary.left = InjectCast(expr->binary.left, commonType);

			if (expr->binary.right->resolvedType != commonType)
				expr->binary.right = InjectCast(expr->binary.right, commonType);

			expr->resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_BOOL);
			return true;
		}

		if (!leftType->IsArithmetic() || !rightType->IsArithmetic())
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Binary operator '{}' requires arithmetic operands, got '{}' and '{}'",
			                           BinaryOperatorToTokenCStr(expr->binary.op), leftType->ToString(), rightType->ToString());
			return false;
		}

		Type* commonType = Type::GetImplicitCommonType(leftType, rightType);
		if (commonType == nullptr)
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Operator '{}' cannot be applied to operands of type '{}' and '{}'",
			                           BinaryOperatorToTokenCStr(expr->binary.op), leftType->ToString(), rightType->ToString());
			return false;
		}

		if (expr->binary.left->resolvedType != commonType)
			expr->binary.left = InjectCast(expr->binary.left, commonType);

		if (expr->binary.right->resolvedType != commonType)
			expr->binary.right = InjectCast(expr->binary.right, commonType);

		expr->resolvedType = commonType;
		return true;
	}

	bool Sema::AnalyzeGroupExpression(Expression* expr, Type* typeHint)
	{
		if (!AnalyzeExpression(expr->group.inner, typeHint))
			return false;

		expr->resolvedType = expr->group.inner->resolvedType;
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

	bool Sema::AnalyzeIncDecExpression(Expression* expr, Type* /*typeHint*/)
	{
		if (!AnalyzeExpression(expr->incdec.operand, nullptr))
			return false;

		if (!IsVariableLValue(expr->incdec.operand))
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Operator '{}' requires a variable operand",
			                           expr->incdec.isIncrement ? "++" : "--");
			return false;
		}

		Type* operandType = expr->incdec.operand->resolvedType;
		ASSERT(operandType != nullptr);

		if (!operandType->IsArithmetic())
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Operator '{}' requires an arithmetic operand, got '{}'",
			                           expr->incdec.isIncrement ? "++" : "--", operandType->ToString());
			return false;
		}

		expr->resolvedType = operandType;
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

		Declaration* functionDecl = sym->declarationRef;
		ASSERT(functionDecl != nullptr);

		if (functionDecl->type == DECLARATION_TYPE_INVALID)
			return false;

		ASSERT(functionDecl->type == DECLARATION_TYPE_FUNCTION);

		expr->resolvedType        = sym->type;
		expr->call.declarationRef = functionDecl;

		const u64 parameterCount = functionDecl->function.parameters.Length();
		const u64 argumentCount  = expr->call.arguments.Length();
		const bool hasNamedArgs  = argumentCount > 0 && expr->call.arguments[0].name;

		if (!hasNamedArgs)
		{
			if (argumentCount != parameterCount)
			{
				m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Function '{}' expects {} arguments, got {}", expr->call.functionName,
				                           parameterCount, argumentCount);
				return false;
			}

			for (u64 i = 0; i < argumentCount; i++)
			{
				if (!AnalyzeCallArgument(&expr->call.arguments[i].expression, functionDecl->function.parameters[i]))
					return false;
			}

			return true;
		}

		Vector<CallExpression::Argument> normalizedArguments =
		    Vector<CallExpression::Argument>::Create(m_ExprAllocator, parameterCount > 0 ? parameterCount : 1);
		for (u64 i = 0; i < parameterCount; i++) normalizedArguments.Emplace();

		for (u64 argumentIndex = 0; argumentIndex < argumentCount; argumentIndex++)
		{
			CallExpression::Argument argument = expr->call.arguments[argumentIndex];

			Declaration* parameterDecl = nullptr;
			u64 parameterIndex         = 0;
			for (; parameterIndex < parameterCount; parameterIndex++)
			{
				Declaration* candidate = functionDecl->function.parameters[parameterIndex];
				ASSERT(candidate != nullptr);
				ASSERT(candidate->type == DECLARATION_TYPE_VARIABLE);

				if (candidate->variable.name == std::string_view(argument.name.Data(), argument.name.Length()))
				{
					parameterDecl = candidate;
					break;
				}
			}

			if (parameterDecl == nullptr)
			{
				m_Diagnostics->ReportError(argument.span, m_TranslationUnit->file, "Function '{}' has no parameter named '{}'",
				                           expr->call.functionName, argument.name);
				return false;
			}

			if (normalizedArguments[parameterIndex].expression != nullptr)
			{
				m_Diagnostics->ReportError(argument.span, m_TranslationUnit->file, "Argument for parameter '{}' was provided more than once",
				                           parameterDecl->variable.name);
				return false;
			}

			if (!AnalyzeCallArgument(&argument.expression, parameterDecl))
				return false;

			normalizedArguments[parameterIndex] = argument;
		}

		for (u64 parameterIndex = 0; parameterIndex < parameterCount; parameterIndex++)
		{
			if (normalizedArguments[parameterIndex].expression != nullptr)
				continue;

			Declaration* parameterDecl = functionDecl->function.parameters[parameterIndex];
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Missing named argument for parameter '{}' in call to '{}'",
			                           parameterDecl->variable.name, expr->call.functionName);
			return false;
		}

		expr->call.arguments = normalizedArguments;

		return true;
	}

	bool Sema::AnalyzeAssignmentExpression(Expression* expr, Type* /*typeHint*/)
	{
		if (!AnalyzeExpression(expr->assignment.left, nullptr))
			return false;

		if (!IsVariableLValue(expr->assignment.left))
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Left-hand side of assignment must be a variable");
			return false;
		}

		Type* leftType = expr->assignment.left->resolvedType;
		ASSERT(leftType != nullptr);

		if (expr->assignment.op == ASSIGNMENT_OPERATOR_PURE)
		{
			if (!AnalyzeExpression(expr->assignment.right, leftType))
				return false;

			if (expr->assignment.right->resolvedType != leftType)
			{
				if (!expr->assignment.right->resolvedType->IsImplicitlyConvertibleTo(leftType))
				{
					m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Cannot implicitly assign value of type '{}' to '{}'",
					                           expr->assignment.right->resolvedType->ToString(), leftType->ToString());
					return false;
				}

				expr->assignment.right = InjectCast(expr->assignment.right, leftType);
			}

			expr->resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_VOID);
			return true;
		}

		if (!AnalyzeExpression(expr->assignment.right, nullptr))
			return false;

		if (!leftType->IsArithmetic())
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file,
			                           "Compound assignment operator '{}' requires an arithmetic left-hand side, got '{}'",
			                           AssignmentOperatorToTokenCStr(expr->assignment.op), leftType->ToString());
			return false;
		}

		Type* rightType = expr->assignment.right->resolvedType;
		ASSERT(rightType != nullptr);

		if (rightType == Type::GetBuiltinType(BUILTIN_TYPE_ABSTRACT_INT))
			TryResolveAbstractIntegerConstantToType(expr->assignment.right, leftType);

		rightType = expr->assignment.right->resolvedType;

		if (!rightType->IsArithmetic())
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file,
			                           "Compound assignment operator '{}' requires an arithmetic right-hand side, got '{}'",
			                           AssignmentOperatorToTokenCStr(expr->assignment.op), rightType->ToString());
			return false;
		}

		Type* commonType = Type::GetImplicitCommonType(leftType, rightType);
		if (commonType == nullptr)
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Operator '{}' cannot be applied to operands of type '{}' and '{}'",
			                           AssignmentOperatorToTokenCStr(expr->assignment.op), leftType->ToString(), rightType->ToString());
			return false;
		}

		if (!commonType->IsImplicitlyConvertibleTo(leftType))
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file,
			                           "Result of operator '{}' has type '{}' which cannot be implicitly assigned to '{}'",
			                           AssignmentOperatorToTokenCStr(expr->assignment.op), commonType->ToString(), leftType->ToString());
			return false;
		}

		if (expr->assignment.right->resolvedType != commonType)
			expr->assignment.right = InjectCast(expr->assignment.right, commonType);

		expr->resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_VOID);
		return true;
	}

	bool Sema::AnalyzeCallArgument(Expression** argumentExpression, Declaration* parameterDecl)
	{
		ASSERT(argumentExpression != nullptr);
		ASSERT(*argumentExpression != nullptr);
		ASSERT(parameterDecl != nullptr);
		ASSERT(parameterDecl->type == DECLARATION_TYPE_VARIABLE);

		Type* parameterType = parameterDecl->variable.type;
		if (!AnalyzeExpression(*argumentExpression, parameterType))
			return false;

		if ((*argumentExpression)->resolvedType == parameterType)
			return true;

		if (!(*argumentExpression)->resolvedType->IsImplicitlyConvertibleTo(parameterType))
		{
			m_Diagnostics->ReportError((*argumentExpression)->span, m_TranslationUnit->file,
			                           "Cannot implicitly cast argument for parameter '{}' from '{}' to '{}'", parameterDecl->variable.name,
			                           (*argumentExpression)->resolvedType->ToString(), parameterType->ToString());
			return false;
		}

		*argumentExpression = InjectCast(*argumentExpression, parameterType);
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
