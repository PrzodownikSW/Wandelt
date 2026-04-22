#pragma once

#include "Wandelt/Ast.hpp"
#include "Wandelt/Defines.hpp"
#include "Wandelt/File.hpp"
#include "Wandelt/Memory.hpp"
#include "Wandelt/String.hpp"
#include "Wandelt/Type.hpp"

namespace Wandelt
{

	enum SymbolKind
	{
		SYMBOL_KIND_INVALID = 0,

		SYMBOL_KIND_VARIABLE,
		SYMBOL_KIND_FUNCTION,

		SYMBOL_KIND_COUNT,
	};

	const char* SymbolKindToCStr(SymbolKind kind);

	struct Symbol
	{
		StringView name;
		SymbolKind kind;
		Type* type;
		Declaration* declarationRef;
		const File* sourceFile;
		u32 scopeDepth;
		bool isUsed;

		Symbol* nextInBucket;
		Symbol* nextInScope;
	};

	struct Scope
	{
		Scope* parent;
		Symbol* firstSymbol;
		u32 depth; // nesting depth (0 = global)
	};

	class SymbolTable
	{
	public:
		static constexpr u32 BucketCount = 128;

		explicit SymbolTable(Allocator* allocator);
		~SymbolTable() = default;

		NONCOPYABLE(SymbolTable);
		NONMOVABLE(SymbolTable);

	public:
		u32 GetScopeDepth() const { return m_ScopeDepth; }
		const Scope* GetCurrentScope() const { return m_CurrentScope; }

	public:
		void PushScope();
		void PopScope();

		Symbol* Insert(StringView name, SymbolKind kind, Type* type, Declaration* decl);
		Symbol* Lookup(StringView name, bool markUsed);

		void DebugPrint() const;

	private:
		Allocator* m_Allocator         = nullptr;
		Symbol* m_Buckets[BucketCount] = {};
		Scope* m_CurrentScope          = nullptr;
		u32 m_ScopeDepth               = 0;
	};

} // namespace Wandelt
