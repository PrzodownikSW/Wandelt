#include "Type.hpp"

namespace Wandelt
{

	const char* TypeKindToCStr(TypeKind kind)
	{
		switch (kind)
		{
		case TYPE_KIND_INVALID:
			ASSERT(false, "Invalid type kind");
			break;

		case TYPE_KIND_BUILTIN:
			return "builtin";

		case TYPE_KIND_FUNCTION:
			return "function";

		case TYPE_KIND_COUNT:
			ASSERT(false, "Invalid type kind");
			break;
		}

		UNREACHABLE();
	}

	const char* BuiltinTypeKindToCStr(BuiltinTypeKind kind)
	{
		switch (kind)
		{
		case BUILTIN_TYPE_INVALID:
			ASSERT(false, "Invalid builtin type kind");
			break;

		case BUILTIN_TYPE_VOID:
			return "void";

		case BUILTIN_TYPE_BOOL:
			return "bool";

		case BUILTIN_TYPE_CHAR:
			return "char";

		case BUILTIN_TYPE_SHORT:
			return "short";

		case BUILTIN_TYPE_INT:
			return "int";

		case BUILTIN_TYPE_LONG:
			return "long";

		case BUILTIN_TYPE_SZ:
			return "sz";

		case BUILTIN_TYPE_INTPTR:
			return "intptr";

		case BUILTIN_TYPE_UCHAR:
			return "uchar";

		case BUILTIN_TYPE_USHORT:
			return "ushort";

		case BUILTIN_TYPE_UINT:
			return "uint";

		case BUILTIN_TYPE_ULONG:
			return "ulong";

		case BUILTIN_TYPE_USZ:
			return "usz";

		case BUILTIN_TYPE_UINTPTR:
			return "uintptr";

		case BUILTIN_TYPE_FLOAT:
			return "float";

		case BUILTIN_TYPE_DOUBLE:
			return "double";

		case BUILTIN_TYPE_STRING:
			return "string";

		case BUILTIN_TYPE_CSTRING:
			return "cstring";

		case BUILTIN_TYPE_RAWPTR:
			return "rawptr";

		case BUILTIN_TYPE_COUNT:
			ASSERT(false, "Invalid builtin type kind");
			break;
		}

		UNREACHABLE();
	}

	enum CastKind : u8
	{
		CAST_KIND_NO = 0,
		CAST_KIND_IMPLICIT,
		CAST_KIND_EXPLICIT,
	};

#define NONE CAST_KIND_NO
#define IMPL CAST_KIND_IMPLICIT
#define EXPL CAST_KIND_EXPLICIT

	// Rows = from-type, cols = to-type.
	static constexpr CastKind s_CastMatrix[BUILTIN_TYPE_COUNT][BUILTIN_TYPE_COUNT] = {
	    //             INV   VOID  BOOL  CHAR  SHORT INT   LONG  SZ    IPTR  UCHR  USHT  UINT  ULNG  USZ   UIPT  FLT   DBL   STR   CSTR  RPTR
	    /* VOID    */ {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
	    /* INVALID */ {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
	    /* BOOL    */ {NONE, NONE, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, NONE, NONE},
	    /* CHAR    */ {NONE, NONE, EXPL, IMPL, IMPL, IMPL, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, NONE, NONE},
	    /* SHORT   */ {NONE, NONE, EXPL, EXPL, IMPL, IMPL, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, NONE, NONE},
	    /* INT     */ {NONE, NONE, EXPL, EXPL, EXPL, IMPL, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, NONE, NONE},
	    /* LONG    */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, NONE, NONE},
	    /* SZ      */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, NONE, NONE},
	    /* INTPTR  */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, NONE, NONE},
	    /* UCHAR   */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, IMPL, IMPL, IMPL, EXPL, EXPL, EXPL, EXPL, NONE, NONE, NONE},
	    /* USHORT  */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, IMPL, IMPL, EXPL, EXPL, EXPL, EXPL, NONE, NONE, NONE},
	    /* UINT    */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, IMPL, EXPL, EXPL, EXPL, EXPL, NONE, NONE, NONE},
	    /* ULONG   */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, EXPL, EXPL, EXPL, EXPL, NONE, NONE, NONE},
	    /* USZ     */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, EXPL, EXPL, EXPL, NONE, NONE, NONE},
	    /* UINTPTR */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, EXPL, EXPL, NONE, NONE, NONE},
	    /* FLOAT   */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, IMPL, NONE, NONE, NONE},
	    /* DOUBLE  */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, NONE, NONE, NONE},
	    /* STRING  */ {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, IMPL, NONE, NONE},
	    /* CSTRING */ {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, IMPL, NONE},
	    /* RAWPTR  */ {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, IMPL},
	};

#undef NONE
#undef IMPL
#undef EXPL

	bool Type::IsImplicitlyConvertibleTo(Type* other)
	{
		ASSERT(this->kind != TYPE_KIND_INVALID && other->kind != TYPE_KIND_INVALID);

		if (this == other)
			return true;

		if (this->kind == TYPE_KIND_BUILTIN && other->kind == TYPE_KIND_BUILTIN)
			return s_CastMatrix[this->basic.kind][other->basic.kind] == CAST_KIND_IMPLICIT;

		return false;
	}

	bool Type::IsExplicitlyConvertibleTo(Type* other)
	{
		ASSERT(this->kind != TYPE_KIND_INVALID && other->kind != TYPE_KIND_INVALID);

		if (this == other)
			return true;

		if (this->kind == TYPE_KIND_BUILTIN && other->kind == TYPE_KIND_BUILTIN)
		{
			CastKind c = s_CastMatrix[this->basic.kind][other->basic.kind];
			return c == CAST_KIND_IMPLICIT || c == CAST_KIND_EXPLICIT;
		}

		return false;
	}

	// no array designators in c++ ...
	static constinit Type g_builtinTypes[BUILTIN_TYPE_COUNT] = {
	    /* INVALID */ Type(),
	    /* VOID    */ Type(BUILTIN_TYPE_VOID, 0, 1, 0),
	    /* BOOL    */ Type(BUILTIN_TYPE_BOOL, 1, 1, TYPE_FLAG_BOOLEAN),
	    /* CHAR    */ Type(BUILTIN_TYPE_CHAR, 1, 1, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED),
	    /* SHORT   */ Type(BUILTIN_TYPE_SHORT, 2, 2, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED),
	    /* INT     */ Type(BUILTIN_TYPE_INT, 4, 4, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED),
	    /* LONG    */ Type(BUILTIN_TYPE_LONG, 8, 8, TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED),
	    /* SZ      */ Type(BUILTIN_TYPE_SZ, sizeof(void*), alignof(void*), TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED | TYPE_FLAG_ARCH_DEP),
	    /* INTPTR  */ Type(BUILTIN_TYPE_INTPTR, sizeof(void*), alignof(void*), TYPE_FLAG_INTEGER | TYPE_FLAG_SIGNED | TYPE_FLAG_ARCH_DEP),
	    /* UCHAR   */ Type(BUILTIN_TYPE_UCHAR, 1, 1, TYPE_FLAG_INTEGER | TYPE_FLAG_UNSIGNED),
	    /* USHORT  */ Type(BUILTIN_TYPE_USHORT, 2, 2, TYPE_FLAG_INTEGER | TYPE_FLAG_UNSIGNED),
	    /* UINT    */ Type(BUILTIN_TYPE_UINT, 4, 4, TYPE_FLAG_INTEGER | TYPE_FLAG_UNSIGNED),
	    /* ULONG   */ Type(BUILTIN_TYPE_ULONG, 8, 8, TYPE_FLAG_INTEGER | TYPE_FLAG_UNSIGNED),
	    /* USZ     */ Type(BUILTIN_TYPE_USZ, sizeof(void*), alignof(void*), TYPE_FLAG_INTEGER | TYPE_FLAG_UNSIGNED | TYPE_FLAG_ARCH_DEP),
	    /* UINTPTR */ Type(BUILTIN_TYPE_UINTPTR, sizeof(void*), alignof(void*), TYPE_FLAG_INTEGER | TYPE_FLAG_UNSIGNED | TYPE_FLAG_ARCH_DEP),
	    /* FLOAT   */ Type(BUILTIN_TYPE_FLOAT, 4, 4, TYPE_FLAG_FLOATING_POINT),
	    /* DOUBLE  */ Type(BUILTIN_TYPE_DOUBLE, 8, 8, TYPE_FLAG_FLOATING_POINT),
	    /* STRING  */ Type(BUILTIN_TYPE_STRING, sizeof(void*) + sizeof(u64), alignof(void*), TYPE_FLAG_STRING),
	    /* CSTRING */ Type(BUILTIN_TYPE_CSTRING, sizeof(void*), alignof(void*), TYPE_FLAG_STRING),
	    /* RAWPTR  */ Type(BUILTIN_TYPE_RAWPTR, sizeof(void*), alignof(void*), 0),
	};

	Type* Type::GetBuiltinType(BuiltinTypeKind kind)
	{
		ASSERT(kind > BUILTIN_TYPE_INVALID && kind < BUILTIN_TYPE_COUNT);
		return &g_builtinTypes[kind];
	}

	Type* Type::TryGetBuiltinType(BuiltinTypeKind kind)
	{
		if (kind > BUILTIN_TYPE_INVALID && kind < BUILTIN_TYPE_COUNT)
			return &g_builtinTypes[kind];

		return nullptr;
	}

	Type* Type::GetImplicitCommonType(Type* a, Type* b)
	{
		ASSERT(a->kind != TYPE_KIND_INVALID && b->kind != TYPE_KIND_INVALID);

		if (a == b)
			return a;

		// If left can widen to right, result is right's type
		if (a->IsImplicitlyConvertibleTo(b))
			return b;

		// If right can widen to left, result is left's type
		if (b->IsImplicitlyConvertibleTo(a))
			return a;

		// No implicit conversion
		return nullptr;
	}

} // namespace Wandelt
