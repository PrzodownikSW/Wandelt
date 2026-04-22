#include "SymbolTable.hpp"

#include <cstdio>

namespace Wandelt
{

	// TODO: move this
	static u64 Fnv1aHash(StringView name, u64 mod)
	{
		constexpr u64 offsetBasis = 14695981039346656037ULL;
		constexpr u64 prime       = 1099511628211ULL;

		u64 hash         = offsetBasis;
		const char* data = name.Data();
		const u64 len    = name.Length();

		for (u64 i = 0; i < len; ++i)
		{
			hash ^= static_cast<u64>(static_cast<u8>(data[i]));
			hash *= prime;
		}

		return hash % mod;
	}

	const char* SymbolKindToCStr(SymbolKind kind)
	{
		switch (kind)
		{
		case SYMBOL_KIND_INVALID:
			ASSERT(false, "Invalid symbol kind");
			break;

		case SYMBOL_KIND_VARIABLE:
			return "var";

		case SYMBOL_KIND_FUNCTION:
			return "fn";

		case SYMBOL_KIND_COUNT:
			ASSERT(false, "Invalid symbol kind");
			break;
		}

		UNREACHABLE();
	}

	SymbolTable::SymbolTable(Allocator* allocator) : m_Allocator(allocator)
	{
	}

	void SymbolTable::PushScope()
	{
		Scope* scope       = m_Allocator->Alloc<Scope>();
		scope->parent      = m_CurrentScope;
		scope->firstSymbol = nullptr;
		scope->depth       = ++m_ScopeDepth;

		m_CurrentScope = scope;
	}

	void SymbolTable::PopScope()
	{
		Scope* scope = m_CurrentScope;
		ASSERT(scope != nullptr, "Cannot pop global scope");

		Symbol* sym = scope->firstSymbol;
		while (sym != nullptr)
		{
			const u64 bucket = Fnv1aHash(sym->name, BucketCount);
			ASSERT(m_Buckets[bucket] == sym, "Symbol table corruption: symbol not at bucket head");

			m_Buckets[bucket] = sym->nextInBucket;
			sym               = sym->nextInScope;
		}

		m_CurrentScope = scope->parent;
		m_ScopeDepth--;
	}

	Symbol* SymbolTable::Insert(StringView name, SymbolKind kind, Type* type, Declaration* decl)
	{
		const u64 bucket = Fnv1aHash(name, BucketCount);

		Symbol* existing = m_Buckets[bucket];
		while (existing != nullptr)
		{
			if (existing->name == static_cast<std::string_view>(name))
				return nullptr;

			existing = existing->nextInBucket;
		}

		Symbol* sym         = m_Allocator->Alloc<Symbol>();
		sym->name           = name;
		sym->kind           = kind;
		sym->type           = type;
		sym->declarationRef = decl;
		sym->sourceFile     = nullptr;
		sym->scopeDepth     = m_ScopeDepth;
		sym->isUsed         = false;

		sym->nextInBucket = m_Buckets[bucket];
		m_Buckets[bucket] = sym;

		sym->nextInScope            = m_CurrentScope->firstSymbol;
		m_CurrentScope->firstSymbol = sym;

		return sym;
	}

	Symbol* SymbolTable::Lookup(StringView name, bool markUsed)
	{
		const u64 bucket = Fnv1aHash(name, BucketCount);
		Symbol* sym      = m_Buckets[bucket];

		while (sym != nullptr)
		{
			if (sym->name == static_cast<std::string_view>(name))
			{
				sym->isUsed = markUsed;
				return sym;
			}

			sym = sym->nextInBucket;
		}

		return nullptr;
	}

	void SymbolTable::DebugPrint() const
	{
		printf("Symbol Table ");
		for (int i = 0; i < 51; i++) putchar('=');
		putchar('\n');
		printf("  Scope depth : %u\n", m_ScopeDepth);

		const Scope* scope = m_CurrentScope;
		while (scope != nullptr)
		{
			printf("\n  Scope (depth %u)\n", scope->depth);
			printf("  ");
			for (int i = 0; i < 66; i++) putchar('-');
			putchar('\n');

			const Symbol* sym = scope->firstSymbol;
			if (sym == nullptr)
			{
				printf("    (empty)\n");
			}

			while (sym != nullptr)
			{
				const char* kindStr = SymbolKindToCStr(sym->kind);
				const char* typeStr = sym->type ? TypeKindToCStr(sym->type->kind) : "???";

				printf("    %-4s %-8s %.*s", kindStr, typeStr, static_cast<int>(sym->name.Length()), sym->name.Data());

				if (sym->isUsed)
					printf("  (used)");
				else
					printf("  (unused)");

				putchar('\n');
				sym = sym->nextInScope;
			}

			scope = scope->parent;
		}

		printf("\n");
		for (int i = 0; i < 68; i++) putchar('=');
		putchar('\n');
	}

} // namespace Wandelt
