#include "Type.hpp"

namespace Wandelt
{

	static Allocator* g_TypeAllocator    = nullptr;
	static Vector<Type*> g_FunctionTypes = {};
	static Vector<Type*> g_ArrayTypes    = {};
	static Vector<Type*> g_SliceTypes    = {};
	static Vector<Type*> g_PointerTypes  = {};

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

		case TYPE_KIND_ARRAY:
			return "array";

		case TYPE_KIND_SLICE:
			return "slice";

		case TYPE_KIND_POINTER:
			return "pointer";

		case TYPE_KIND_COUNT:
			ASSERT(false, "Invalid type kind");
			break;
		}

		UNREACHABLE();
	}

	void Type::Initialize(Allocator* allocator)
	{
		g_TypeAllocator = allocator;

		g_FunctionTypes = Vector<Type*>::Create(g_TypeAllocator, 8);
		g_ArrayTypes    = Vector<Type*>::Create(g_TypeAllocator, 8);
		g_SliceTypes    = Vector<Type*>::Create(g_TypeAllocator, 8);
		g_PointerTypes  = Vector<Type*>::Create(g_TypeAllocator, 8);
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

		case BUILTIN_TYPE_ABSTRACT_INT:
			return "integer";

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

		case BUILTIN_TYPE_NULLPTR:
			return "null";

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

	static_assert(BUILTIN_TYPE_COUNT == 22,
	              "If you add a new builtin type, make sure to update the cast matrix below IN PROPER ORDER otherwise all hell will break loose");

	// clang-format off

	// Rows = from-type, cols = to-type.
	static constexpr CastKind s_CastMatrix[BUILTIN_TYPE_COUNT][BUILTIN_TYPE_COUNT] = {
	    //             INV   VOID  BOOL  CHAR  SHORT INT   LONG  SZ    IPTR  UCHR  USHT  UINT  ULNG  USZ   UIPT  ABINT FLT   DBL   STR   CSTR  RPTR  NULL
	    /* VOID    */ {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
	    /* INVALID */ {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE},
	    /* BOOL    */ {NONE, NONE, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, EXPL, EXPL, NONE, NONE, NONE, NONE},
	    /* CHAR    */ {NONE, NONE, EXPL, IMPL, IMPL, IMPL, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, EXPL, EXPL, NONE, NONE, NONE, NONE},
	    /* SHORT   */ {NONE, NONE, EXPL, EXPL, IMPL, IMPL, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, EXPL, EXPL, NONE, NONE, NONE, NONE},
	    /* INT     */ {NONE, NONE, EXPL, EXPL, EXPL, IMPL, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, EXPL, EXPL, NONE, NONE, NONE, NONE},
	    /* LONG    */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, EXPL, EXPL, NONE, NONE, NONE, NONE},
	    /* SZ      */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, EXPL, EXPL, NONE, NONE, NONE, NONE},
	    /* INTPTR  */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, EXPL, EXPL, NONE, NONE, NONE, NONE},
	    /* UCHAR   */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, IMPL, IMPL, IMPL, EXPL, EXPL, NONE, EXPL, EXPL, NONE, NONE, NONE, NONE},
	    /* USHORT  */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, IMPL, IMPL, EXPL, EXPL, NONE, EXPL, EXPL, NONE, NONE, NONE, NONE},
	    /* UINT    */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, IMPL, EXPL, EXPL, NONE, EXPL, EXPL, NONE, NONE, NONE, NONE},
	    /* ULONG   */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, EXPL, EXPL, NONE, EXPL, EXPL, NONE, NONE, NONE, NONE},
	    /* USZ     */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, EXPL, NONE, EXPL, EXPL, NONE, NONE, NONE, NONE},
	    /* UINTPTR */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, IMPL, NONE, EXPL, EXPL, NONE, NONE, NONE, NONE},
	    /* ABINT   */ {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, IMPL, NONE, NONE, NONE, NONE, NONE, NONE},
	    /* FLOAT   */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, IMPL, IMPL, NONE, NONE, NONE, NONE},
	    /* DOUBLE  */ {NONE, NONE, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, EXPL, NONE, EXPL, IMPL, NONE, NONE, NONE, NONE},
	    /* STRING  */ {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, IMPL, NONE, NONE, NONE},
	    /* CSTRING */ {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, IMPL, NONE, NONE},
	    /* RAWPTR  */ {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, IMPL, NONE},
	    /* NULL    */ {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, IMPL},
	};

	// clang-format on

#undef NONE
#undef IMPL
#undef EXPL

	bool Type::IsImplicitlyConvertibleTo(Type* other)
	{
		ASSERT(this->kind != TYPE_KIND_INVALID && other->kind != TYPE_KIND_INVALID);

		if (this == other)
			return true;

		if (this->kind == TYPE_KIND_BUILTIN && this->basic.kind == BUILTIN_TYPE_ABSTRACT_INT)
			return false;

		if (this->IsNullPtrType() && other->IsPointerLike())
			return true;

		if (this->IsArrayType() && other->IsSliceType() && this->array.elementType == other->slice.elementType)
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

		if (this->kind == TYPE_KIND_BUILTIN && this->basic.kind == BUILTIN_TYPE_ABSTRACT_INT)
			return other->IsArithmetic();

		if (this->IsNullPtrType() && other->IsPointerLike())
			return true;

		if (this->kind == TYPE_KIND_BUILTIN && other->kind == TYPE_KIND_BUILTIN)
		{
			CastKind c = s_CastMatrix[this->basic.kind][other->basic.kind];
			return c == CAST_KIND_IMPLICIT || c == CAST_KIND_EXPLICIT;
		}

		return false;
	}

	bool Type::CanRepresentIntegerConstant(u64 value) const
	{
		if (kind != TYPE_KIND_BUILTIN)
			return false;

		switch (basic.kind)
		{
		case BUILTIN_TYPE_CHAR:
		case BUILTIN_TYPE_SHORT:
		case BUILTIN_TYPE_INT:
		case BUILTIN_TYPE_LONG:
		case BUILTIN_TYPE_SZ:
		case BUILTIN_TYPE_INTPTR: {
			const u32 bits = sizeInBytes * 8;
			if (bits >= 64)
				return value <= (u64)std::numeric_limits<i64>::max();

			return value <= ((1ULL << (bits - 1)) - 1);
		}

		case BUILTIN_TYPE_UCHAR:
		case BUILTIN_TYPE_USHORT:
		case BUILTIN_TYPE_UINT:
		case BUILTIN_TYPE_ULONG:
		case BUILTIN_TYPE_USZ:
		case BUILTIN_TYPE_UINTPTR: {
			const u32 bits = sizeInBytes * 8;
			if (bits >= 64)
				return true;

			return value <= ((1ULL << bits) - 1);
		}

		case BUILTIN_TYPE_ABSTRACT_INT:
			return true;

		case BUILTIN_TYPE_FLOAT: {
			f32 converted = (f32)value;
			return (u64)converted == value;
		}

		case BUILTIN_TYPE_DOUBLE: {
			f64 converted = (f64)value;
			return (u64)converted == value;
		}

		default:
			return false;
		}
	}

	std::string Type::ToString() const
	{
		switch (kind)
		{
		case TYPE_KIND_INVALID:
			ASSERT(false, "Invalid type kind");
			return "<invalid>";
		case TYPE_KIND_BUILTIN:
			return BuiltinTypeKindToCStr(basic.kind);
		case TYPE_KIND_FUNCTION: {
			std::string result = "fn(";
			for (u64 i = 0; i < fn.params.Length(); i++)
			{
				if (i > 0)
					result += ", ";

				result += fn.params[i]->ToString();
			}

			if (fn.isVariadic)
			{
				if (!fn.params.IsEmpty())
					result += ", ";

				result += "...";
			}

			result += ") ";
			result += fn.returnType->ToString();
			return result;
		}
		case TYPE_KIND_ARRAY: {
			std::vector<u64> dimensions = {};
			const Type* current         = this;
			while (current->kind == TYPE_KIND_ARRAY)
			{
				dimensions.push_back(current->array.length);
				current = current->array.elementType;
			}

			std::string result = current->ToString();
			for (u64 dimension : dimensions) result += "[" + std::to_string(dimension) + "]";

			return result;
		}
		case TYPE_KIND_SLICE:
			return slice.elementType->ToString() + "[]";
		case TYPE_KIND_POINTER:
			return pointer.pointeeType->ToString() + "^";
		case TYPE_KIND_COUNT:
			ASSERT(false, "Invalid type kind");
			return "<invalid>";
		}

		UNREACHABLE();
	}

	static_assert(
	    BUILTIN_TYPE_COUNT == 22,
	    "If you add a new builtin type, make sure to update the g_builtinTypes array below IN PROPER ORDER otherwise all hell will break loose");

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
	    /* ABINT   */ Type(BUILTIN_TYPE_ABSTRACT_INT, 0, 1, TYPE_FLAG_INTEGER),
	    /* FLOAT   */ Type(BUILTIN_TYPE_FLOAT, 4, 4, TYPE_FLAG_FLOATING_POINT),
	    /* DOUBLE  */ Type(BUILTIN_TYPE_DOUBLE, 8, 8, TYPE_FLAG_FLOATING_POINT),
	    /* STRING  */ Type(BUILTIN_TYPE_STRING, sizeof(void*) + sizeof(u64), alignof(void*), TYPE_FLAG_STRING),
	    /* CSTRING */ Type(BUILTIN_TYPE_CSTRING, sizeof(void*), alignof(void*), TYPE_FLAG_STRING),
	    /* RAWPTR  */ Type(BUILTIN_TYPE_RAWPTR, sizeof(void*), alignof(void*), 0),
	    /* NULLPTR */ Type(BUILTIN_TYPE_NULLPTR, 0, 1, 0),
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

	static bool FunctionTypeMatches(Type* functionType, Type* returnType, const Vector<Type*>& paramTypes, bool isVariadic)
	{
		ASSERT(functionType != nullptr);
		ASSERT(functionType->kind == TYPE_KIND_FUNCTION);

		if (functionType->fn.returnType != returnType)
			return false;

		if (functionType->fn.isVariadic != isVariadic)
			return false;

		if (functionType->fn.params.Length() != paramTypes.Length())
			return false;

		for (u64 i = 0; i < paramTypes.Length(); i++)
		{
			if (functionType->fn.params[i] != paramTypes[i])
				return false;
		}

		return true;
	}

	static bool ArrayTypeMatches(Type* arrayType, Type* elementType, u64 length)
	{
		ASSERT(arrayType != nullptr);
		ASSERT(arrayType->kind == TYPE_KIND_ARRAY);

		return arrayType->array.elementType == elementType && arrayType->array.length == length;
	}

	static bool SliceTypeMatches(Type* sliceType, Type* elementType)
	{
		ASSERT(sliceType != nullptr);
		ASSERT(sliceType->kind == TYPE_KIND_SLICE);

		return sliceType->slice.elementType == elementType;
	}

	Type* Type::GetFunctionType(Type* returnType, const Vector<Type*>& paramTypes, bool isVariadic)
	{
		ASSERT(returnType != nullptr);
		ASSERT(returnType->kind != TYPE_KIND_INVALID);

		for (Type* functionType : g_FunctionTypes)
		{
			if (FunctionTypeMatches(functionType, returnType, paramTypes, isVariadic))
				return functionType;
		}

		Type* functionType             = static_cast<Type*>(g_TypeAllocator->Alloc(sizeof(Type)));
		functionType->kind             = TYPE_KIND_FUNCTION;
		functionType->sizeInBytes      = sizeof(void*);
		functionType->alignInBytes     = alignof(void*);
		functionType->fn.returnType    = returnType;
		functionType->fn.isVariadic    = isVariadic;
		functionType->fn.params.m_Data = nullptr;

		if (!paramTypes.IsEmpty())
		{
			functionType->fn.params = Vector<Type*>::Create(g_TypeAllocator, paramTypes.Length());
			for (Type* paramType : paramTypes) functionType->fn.params.Push(paramType);
		}

		g_FunctionTypes.Push(functionType);

		return functionType;
	}

	Type* Type::GetArrayType(Type* elementType, u64 length)
	{
		ASSERT(elementType != nullptr);
		ASSERT(elementType->kind != TYPE_KIND_INVALID);

		for (Type* arrayType : g_ArrayTypes)
		{
			if (ArrayTypeMatches(arrayType, elementType, length))
				return arrayType;
		}

		const u64 sizeInBytes = static_cast<u64>(elementType->sizeInBytes) * length;
		ASSERT(sizeInBytes <= std::numeric_limits<u32>::max(), "Array type is too large");

		Type* arrayType              = static_cast<Type*>(g_TypeAllocator->Alloc(sizeof(Type)));
		arrayType->kind              = TYPE_KIND_ARRAY;
		arrayType->sizeInBytes       = static_cast<u32>(sizeInBytes);
		arrayType->alignInBytes      = elementType->alignInBytes;
		arrayType->array.length      = length;
		arrayType->array.elementType = elementType;

		g_ArrayTypes.Push(arrayType);

		return arrayType;
	}

	Type* Type::GetSliceType(Type* elementType)
	{
		ASSERT(elementType != nullptr);
		ASSERT(elementType->kind != TYPE_KIND_INVALID);

		for (Type* sliceType : g_SliceTypes)
		{
			if (SliceTypeMatches(sliceType, elementType))
				return sliceType;
		}

		Type* sliceType              = static_cast<Type*>(g_TypeAllocator->Alloc(sizeof(Type)));
		sliceType->kind              = TYPE_KIND_SLICE;
		sliceType->sizeInBytes       = sizeof(void*) + sizeof(u64);
		sliceType->alignInBytes      = alignof(void*);
		sliceType->slice.elementType = elementType;

		g_SliceTypes.Push(sliceType);

		return sliceType;
	}

	static bool PointerTypeMatches(Type* pointerType, Type* pointeeType)
	{
		ASSERT(pointerType != nullptr);
		ASSERT(pointerType->kind == TYPE_KIND_POINTER);

		return pointerType->pointer.pointeeType == pointeeType;
	}

	Type* Type::GetPointerType(Type* pointeeType)
	{
		ASSERT(pointeeType != nullptr);
		ASSERT(pointeeType->kind != TYPE_KIND_INVALID);

		for (Type* pointerType : g_PointerTypes)
		{
			if (PointerTypeMatches(pointerType, pointeeType))
				return pointerType;
		}

		Type* pointerType                = static_cast<Type*>(g_TypeAllocator->Alloc(sizeof(Type)));
		pointerType->kind                = TYPE_KIND_POINTER;
		pointerType->sizeInBytes         = sizeof(void*);
		pointerType->alignInBytes        = alignof(void*);
		pointerType->pointer.pointeeType = pointeeType;

		g_PointerTypes.Push(pointerType);

		return pointerType;
	}

	Type* Type::GetDefaultTypeForIntegerConstant(u64 value)
	{
		if (value <= (u64)std::numeric_limits<i32>::max())
			return GetBuiltinType(BUILTIN_TYPE_INT);

		return GetBuiltinType(BUILTIN_TYPE_ABSTRACT_INT);
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
