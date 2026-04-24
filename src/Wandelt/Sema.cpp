#include "Sema.hpp"

namespace Wandelt
{
	static bool IsAssignableLValue(Expression* expr)
	{
		ASSERT(expr != nullptr);

		if (expr->type == EXPRESSION_TYPE_IDENTIFIER)
			return expr->identifier.declarationRef != nullptr && expr->identifier.declarationRef->type == DECLARATION_TYPE_VARIABLE;

		if (expr->type == EXPRESSION_TYPE_INDEX)
			return IsAssignableLValue(expr->index.target);

		if (expr->type == EXPRESSION_TYPE_UNARY && expr->unary.op == UNARY_OPERATOR_DEREF)
			return true;

		return false;
	}

	static bool IsPointerEqualityOperand(Type* type)
	{
		return type != nullptr && (type->IsPointerLike() || type->IsNullPtrType());
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
				Vector<Type*> parameterTypes = {};
				if (!decl->function.parameters.IsEmpty())
				{
					parameterTypes = Vector<Type*>::Create(m_DeclAllocator, decl->function.parameters.Length());
					for (Declaration* parameterDecl : decl->function.parameters)
					{
						ASSERT(parameterDecl != nullptr);
						ASSERT(parameterDecl->type == DECLARATION_TYPE_VARIABLE);

						parameterTypes.Push(parameterDecl->variable.type);
					}
				}

				Type* functionType = Type::GetFunctionType(decl->function.returnType, parameterTypes, false);
				Symbol* sym        = m_SymbolTable.Insert(decl->function.name, SYMBOL_KIND_FUNCTION, functionType, decl);
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
		case STATEMENT_TYPE_IF:
			return AnalyzeIfStatement(stmt);
		case STATEMENT_TYPE_WHILE:
			return AnalyzeWhileStatement(stmt);
		case STATEMENT_TYPE_FOR:
			return AnalyzeForStatement(stmt);
		case STATEMENT_TYPE_BREAK:
			return AnalyzeBreakStatement(stmt);
		case STATEMENT_TYPE_CONTINUE:
			return AnalyzeContinueStatement(stmt);
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

		if (!ExpressionHasSideEffect(expr))
		{
			m_Diagnostics->ReportError(
			    stmt->span, m_TranslationUnit->file,
			    "Expression statements without side effects are not allowed; the expression of this statement has no effect and can be removed");
			return false;
		}

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

	bool Sema::AnalyzeIfStatement(Statement* stmt)
	{
		Type* boolType = Type::GetBuiltinType(BUILTIN_TYPE_BOOL);

		bool success = true;

		if (!AnalyzeExpression(stmt->ifStmt.condition, nullptr))
		{
			success = false;
		}
		else if (stmt->ifStmt.condition->resolvedType != boolType)
		{
			m_Diagnostics->ReportError(stmt->ifStmt.condition->span, m_TranslationUnit->file,
			                           "If-statement condition must be of type 'bool', got '{}'", stmt->ifStmt.condition->resolvedType->ToString());
			success = false;
		}

		if (!AnalyzeStatement(stmt->ifStmt.thenBlock))
			success = false;

		if (stmt->ifStmt.elseBranch != nullptr && !AnalyzeStatement(stmt->ifStmt.elseBranch))
			success = false;

		return success;
	}

	bool Sema::AnalyzeWhileStatement(Statement* stmt)
	{
		Type* boolType = Type::GetBuiltinType(BUILTIN_TYPE_BOOL);

		bool success = true;

		if (!AnalyzeExpression(stmt->whileStmt.condition, nullptr))
		{
			success = false;
		}
		else if (stmt->whileStmt.condition->resolvedType != boolType)
		{
			m_Diagnostics->ReportError(stmt->whileStmt.condition->span, m_TranslationUnit->file,
			                           "While-statement condition must be of type 'bool', got '{}'",
			                           stmt->whileStmt.condition->resolvedType->ToString());
			success = false;
		}

		m_LoopDepth++;

		if (!AnalyzeStatement(stmt->whileStmt.body))
			success = false;

		m_LoopDepth--;

		return success;
	}

	bool Sema::AnalyzeForStatement(Statement* stmt)
	{
		Type* boolType = Type::GetBuiltinType(BUILTIN_TYPE_BOOL);

		m_SymbolTable.PushScope();
		defer(m_SymbolTable.PopScope());

		bool success = true;

		if (!AnalyzeStatement(stmt->forStmt.init))
			success = false;

		if (!AnalyzeExpression(stmt->forStmt.condition, nullptr))
		{
			success = false;
		}
		else if (stmt->forStmt.condition->resolvedType != boolType)
		{
			m_Diagnostics->ReportError(stmt->forStmt.condition->span, m_TranslationUnit->file,
			                           "For-statement condition must be of type 'bool', got '{}'", stmt->forStmt.condition->resolvedType->ToString());
			success = false;
		}

		if (!AnalyzeExpression(stmt->forStmt.increment, nullptr))
		{
			success = false;
		}
		else if (!ExpressionHasSideEffect(stmt->forStmt.increment))
		{
			m_Diagnostics->ReportError(stmt->forStmt.increment->span, m_TranslationUnit->file, "For-statement increment expression has no effect");
			success = false;
		}

		m_LoopDepth++;
		if (!AnalyzeStatement(stmt->forStmt.body))
			success = false;
		m_LoopDepth--;

		return success;
	}

	bool Sema::AnalyzeBreakStatement(Statement* stmt)
	{
		if (m_LoopDepth == 0)
		{
			m_Diagnostics->ReportError(stmt->span, m_TranslationUnit->file, "'break' statement is only allowed inside a loop");
			return false;
		}

		return true;
	}

	bool Sema::AnalyzeContinueStatement(Statement* stmt)
	{
		if (m_LoopDepth == 0)
		{
			m_Diagnostics->ReportError(stmt->span, m_TranslationUnit->file, "'continue' statement is only allowed inside a loop");
			return false;
		}

		return true;
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

		if (decl->function.returnType != Type::GetBuiltinType(BUILTIN_TYPE_VOID))
		{
			ASSERT(decl->function.body->type == STATEMENT_TYPE_BLOCK);

			if (!StatementAlwaysReturns(decl->function.body))
			{
				m_Diagnostics->ReportError(decl->span, m_TranslationUnit->file,
				                           "Function '{}' with non-void return type must return a value on every control path", decl->function.name);
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

		case EXPRESSION_TYPE_INTRINSIC:
			return AnalyzeIntrinsicExpression(expr, typeHint);

		case EXPRESSION_TYPE_CALL:
			return AnalyzeCallExpression(expr, typeHint);

		case EXPRESSION_TYPE_CAST:
			return AnalyzeCastExpression(expr, typeHint);

		case EXPRESSION_TYPE_INCDEC:
			return AnalyzeIncDecExpression(expr, typeHint);

		case EXPRESSION_TYPE_ASSIGNMENT:
			return AnalyzeAssignmentExpression(expr, typeHint);

		case EXPRESSION_TYPE_ARRAY_LITERAL:
			return AnalyzeArrayLiteralExpression(expr, typeHint);

		case EXPRESSION_TYPE_INDEX:
			return AnalyzeIndexExpression(expr, typeHint);

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
			if (typeHint != nullptr && typeHint->CanRepresentIntegerConstant(expr->constant.integerValue))
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
		else if (expr->constant.kind == CONSTANT_KIND_NULL)
		{
			if (typeHint != nullptr && typeHint->IsPointerLike())
				resolvedType = typeHint;
			else
				resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_NULLPTR);
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
		switch (expr->unary.op)
		{
		case UNARY_OPERATOR_NEGATE:
			if (typeHint != nullptr && typeHint->IsArithmetic())
				operandHint = typeHint;
			break;

		case UNARY_OPERATOR_ADDRESS_OF:
			break;

		case UNARY_OPERATOR_DEREF:
			if (typeHint != nullptr)
				operandHint = Type::GetPointerType(typeHint);
			break;

		case UNARY_OPERATOR_INVALID:
		case UNARY_OPERATOR_COUNT:
			ASSERT(false, "Unhandled unary operator: %i", expr->unary.op);
			return false;
		}

		if (!AnalyzeExpression(expr->unary.operand, operandHint))
			return false;

		Type* operandType = expr->unary.operand->resolvedType;
		ASSERT(operandType != nullptr);

		switch (expr->unary.op)
		{
		case UNARY_OPERATOR_NEGATE:
			if (!operandType->IsArithmetic())
			{
				m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Unary operator '{}' requires an arithmetic operand, got '{}'",
				                           UnaryOperatorToTokenCStr(expr->unary.op), operandType->ToString());
				return false;
			}

			expr->resolvedType = operandType;
			return true;

		case UNARY_OPERATOR_ADDRESS_OF:
			if (!IsAssignableLValue(expr->unary.operand))
			{
				m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Unary operator '{}' requires an addressable operand",
				                           UnaryOperatorToTokenCStr(expr->unary.op));
				return false;
			}

			expr->resolvedType = Type::GetPointerType(operandType);
			return true;

		case UNARY_OPERATOR_DEREF:
			if (!operandType->IsPointerType())
			{
				m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Unary operator '{}' requires a typed pointer operand, got '{}'",
				                           UnaryOperatorToTokenCStr(expr->unary.op), operandType->ToString());
				return false;
			}

			expr->resolvedType = operandType->pointer.pointeeType;
			return true;

		case UNARY_OPERATOR_INVALID:
		case UNARY_OPERATOR_COUNT:
			ASSERT(false, "Unhandled unary operator: %i", expr->unary.op);
			return false;
		}

		ASSERT(false, "Unhandled unary operator: %i", expr->unary.op);
		return false;
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

			if (IsPointerEqualityOperand(leftType) && IsPointerEqualityOperand(rightType))
			{
				if (leftType->IsNullPtrType() && rightType->IsPointerLike())
				{
					expr->binary.left  = InjectCast(expr->binary.left, rightType);
					expr->resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_BOOL);
					return true;
				}

				if (rightType->IsNullPtrType() && leftType->IsPointerLike())
				{
					expr->binary.right = InjectCast(expr->binary.right, leftType);
					expr->resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_BOOL);
					return true;
				}
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

		if (sym->kind == SYMBOL_KIND_FUNCTION)
		{
			expr->resolvedType = sym->type->fn.returnType;
		}
		else
		{
			expr->resolvedType = sym->type;
		}

		expr->identifier.declarationRef = sym->declarationRef;

		return true;
	}

	bool Sema::AnalyzeIntrinsicExpression(Expression* expr, Type* /*typeHint*/)
	{
		switch (expr->intrinsic.kind)
		{
		case INTRINSIC_KIND_LEN:
			if (expr->intrinsic.arguments.Length() != 1)
			{
				m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Intrinsic '{}' expects exactly 1 argument, got {}",
				                           IntrinsicKindToCStr(expr->intrinsic.kind), expr->intrinsic.arguments.Length());
				return false;
			}

			if (!AnalyzeExpression(expr->intrinsic.arguments[0], nullptr))
				return false;

			if (!expr->intrinsic.arguments[0]->resolvedType->IsArrayType() && !expr->intrinsic.arguments[0]->resolvedType->IsSliceType())
			{
				m_Diagnostics->ReportError(expr->intrinsic.arguments[0]->span, m_TranslationUnit->file,
				                           "Intrinsic '{}' requires an array or slice argument, got '{}'", IntrinsicKindToCStr(expr->intrinsic.kind),
				                           expr->intrinsic.arguments[0]->resolvedType->ToString());
				return false;
			}

			expr->resolvedType = Type::GetBuiltinType(BUILTIN_TYPE_SZ);
			return true;

		case INTRINSIC_KIND_INVALID:
		case INTRINSIC_KIND_COUNT:
			ASSERT(false, "Unhandled intrinsic kind: %i", expr->intrinsic.kind);
			return false;
		}

		ASSERT(false, "Unhandled intrinsic kind: %i", expr->intrinsic.kind);
		return false;
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

		if (!IsAssignableLValue(expr->incdec.operand))
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

		Type* functionType        = sym->type;
		expr->resolvedType        = functionType->fn.returnType;
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

		if (!IsAssignableLValue(expr->assignment.left))
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

	bool Sema::AnalyzeArrayLiteralExpression(Expression* expr, Type* typeHint)
	{
		if (typeHint == nullptr || !typeHint->IsArrayType())
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Array literal requires an expected array type");
			return false;
		}

		const u64 expectedLength = typeHint->array.length;
		const u64 explicitCount  = expr->arrayLiteral.items.Length();

		if (expr->arrayLiteral.repeatLastElement)
		{
			if (explicitCount == 0)
			{
				m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Array fill syntax requires at least one explicit element");
				return false;
			}

			if (explicitCount > expectedLength)
			{
				m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file,
				                           "Array literal provides {} explicit elements for array type '{}' with length {}", explicitCount,
				                           typeHint->ToString(), expectedLength);
				return false;
			}
		}
		else if (explicitCount != expectedLength)
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Array literal for type '{}' must initialize exactly {} elements, got {}",
			                           typeHint->ToString(), expectedLength, explicitCount);
			return false;
		}

		for (u64 i = 0; i < explicitCount; i++)
		{
			Expression* item = expr->arrayLiteral.items[i];
			if (!AnalyzeExpression(item, typeHint->array.elementType))
				return false;

			if (item->resolvedType == typeHint->array.elementType)
				continue;

			if (!item->resolvedType->IsImplicitlyConvertibleTo(typeHint->array.elementType))
			{
				m_Diagnostics->ReportError(item->span, m_TranslationUnit->file, "Cannot implicitly cast array element {} from '{}' to '{}'", i,
				                           item->resolvedType->ToString(), typeHint->array.elementType->ToString());
				return false;
			}

			expr->arrayLiteral.items[i] = InjectCast(item, typeHint->array.elementType);
		}

		expr->resolvedType = typeHint;
		return true;
	}

	bool Sema::AnalyzeIndexExpression(Expression* expr, Type* /*typeHint*/)
	{
		if (!AnalyzeExpression(expr->index.target, nullptr))
			return false;

		Type* targetType = expr->index.target->resolvedType;
		ASSERT(targetType != nullptr);

		if (!targetType->IsArrayType() && !targetType->IsSliceType())
		{
			m_Diagnostics->ReportError(expr->span, m_TranslationUnit->file, "Indexing requires an array or slice operand, got '{}'",
			                           targetType->ToString());
			return false;
		}

		Type* indexHint = Type::GetBuiltinType(BUILTIN_TYPE_SZ);
		if (!AnalyzeExpression(expr->index.index, indexHint))
			return false;

		Type* indexType = expr->index.index->resolvedType;
		ASSERT(indexType != nullptr);

		if (!indexType->IsInteger())
		{
			m_Diagnostics->ReportError(expr->index.index->span, m_TranslationUnit->file, "Array index must be an integer, got '{}'",
			                           indexType->ToString());
			return false;
		}

		if (targetType->IsArrayType() && expr->index.index->type == EXPRESSION_TYPE_CONSTANT)
		{
			u64 indexValue = expr->index.index->constant.integerValue;
			if (indexValue >= targetType->array.length)
			{
				m_Diagnostics->ReportError(expr->index.index->span, m_TranslationUnit->file,
				                           "Array index {} is out of bounds for array type '{}' with length {}, valid indices are from 0 to {}",
				                           indexValue, targetType->ToString(), targetType->array.length, targetType->array.length - 1);
				return false;
			}
		}

		expr->resolvedType = targetType->IsArrayType() ? targetType->array.elementType : targetType->slice.elementType;
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

	bool Sema::StatementAlwaysReturns(const Statement* stmt)
	{
		ASSERT(stmt);

		switch (stmt->type)
		{
		case STATEMENT_TYPE_RETURN:
			return true;

		case STATEMENT_TYPE_BLOCK:
			for (const Statement* inner : stmt->block.statements)
			{
				if (StatementAlwaysReturns(inner))
					return true;
			}
			return false;

		case STATEMENT_TYPE_IF:
			if (stmt->ifStmt.elseBranch == nullptr)
				return false;
			return StatementAlwaysReturns(stmt->ifStmt.thenBlock) && StatementAlwaysReturns(stmt->ifStmt.elseBranch);

		case STATEMENT_TYPE_INVALID:
		case STATEMENT_TYPE_DECLARATION:
		case STATEMENT_TYPE_EXPRESSION:
		case STATEMENT_TYPE_WHILE:
		case STATEMENT_TYPE_FOR:
		case STATEMENT_TYPE_BREAK:
		case STATEMENT_TYPE_CONTINUE:
			return false;

		case STATEMENT_TYPE_COUNT:
			ASSERT(false, "Invalid statement type!");
			return false;
		}

		ASSERT(false, "Unhandled statement type: %i", stmt->type);
		return false;
	}

	bool Sema::ExpressionHasSideEffect(const Expression* expr)
	{
		ASSERT(expr);

		switch (expr->type)
		{
		case EXPRESSION_TYPE_INVALID:
			ASSERT(false, "Invalid expression type!");
			return false;

		case EXPRESSION_TYPE_CALL:
		case EXPRESSION_TYPE_ASSIGNMENT:
		case EXPRESSION_TYPE_INCDEC:
			return true;

		case EXPRESSION_TYPE_CONSTANT:
		case EXPRESSION_TYPE_UNARY:
		case EXPRESSION_TYPE_BINARY:
		case EXPRESSION_TYPE_GROUP:
		case EXPRESSION_TYPE_IDENTIFIER:
		case EXPRESSION_TYPE_INTRINSIC:
		case EXPRESSION_TYPE_CAST:
		case EXPRESSION_TYPE_ARRAY_LITERAL:
		case EXPRESSION_TYPE_INDEX:
			return false;

		case EXPRESSION_TYPE_COUNT:
			ASSERT(false, "Invalid expression type!");
			return false;
		}

		ASSERT(false, "Unhandled expression type: %i", expr->type);
		return false;
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
