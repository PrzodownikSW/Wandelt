#pragma once

#include "Wandelt/Defines.hpp"
#include "Wandelt/Vector.hpp"

namespace Wandelt
{

	enum TypeKind
	{
		TYPE_KIND_INVALID = 0,

		TYPE_KIND_BUILTIN,
		TYPE_KIND_FUNCTION,

		TYPE_KIND_COUNT,
	};

	const char* TypeKindToCStr(TypeKind kind);

	enum BuiltinTypeKind
	{
		BUILTIN_TYPE_INVALID = 0,

		BUILTIN_TYPE_VOID,    // 0 bits
		BUILTIN_TYPE_BOOL,    // 1 bit
		BUILTIN_TYPE_CHAR,    // 8 bits
		BUILTIN_TYPE_SHORT,   // 16 bits
		BUILTIN_TYPE_INT,     // 32 bits
		BUILTIN_TYPE_LONG,    // 64 bits
		BUILTIN_TYPE_SZ,      // 32 or 64 bits depending on the platform
		BUILTIN_TYPE_INTPTR,  // 32 or 64 bits depending on the platform
		BUILTIN_TYPE_UCHAR,   // 8 bits
		BUILTIN_TYPE_USHORT,  // 16 bits
		BUILTIN_TYPE_UINT,    // 32 bits
		BUILTIN_TYPE_ULONG,   // 64 bits
		BUILTIN_TYPE_USZ,     // 32 or 64 bits depending on the platform
		BUILTIN_TYPE_UINTPTR, // 32 or 64 bits depending on the platform
		BUILTIN_TYPE_FLOAT,   // 32 bits, IEEE 754 single precision
		BUILTIN_TYPE_DOUBLE,  // 64 bits, IEEE 754 double precision
		BUILTIN_TYPE_STRING,  // ptr to data and length
		BUILTIN_TYPE_CSTRING, // ptr to null-terminated data
		BUILTIN_TYPE_RAWPTR,  // ptr to something

		BUILTIN_TYPE_COUNT,
	};

	const char* BuiltinTypeKindToCStr(BuiltinTypeKind kind);

	enum TypeFlags
	{
		TYPE_FLAG_NONE = 0,

		TYPE_FLAG_BOOLEAN        = BIT(0),
		TYPE_FLAG_INTEGER        = BIT(1),
		TYPE_FLAG_SIGNED         = BIT(2),
		TYPE_FLAG_UNSIGNED       = BIT(3),
		TYPE_FLAG_FLOATING_POINT = BIT(4),
		TYPE_FLAG_STRING         = BIT(5),
		TYPE_FLAG_ARCH_DEP       = BIT(6),
	};

	struct BuiltinType
	{
		BuiltinTypeKind kind;
		u32 flags;
	};

	struct FunctionType
	{
		Vector<struct Type*> params;
		bool isVariadic;
	};

	struct Type
	{
	public:
		constexpr Type() : kind(TYPE_KIND_INVALID), sizeInBytes(0), alignInBytes(0), basic{} {}
		constexpr Type(BuiltinTypeKind builtinKind, u32 sizeInBytes, u32 alignInBytes, u32 flags)
		    : kind(TYPE_KIND_BUILTIN), sizeInBytes(sizeInBytes), alignInBytes(alignInBytes), basic{builtinKind, flags}
		{
		}

	public:
		inline bool IsBuiltinType() { return kind == TYPE_KIND_BUILTIN; }
		inline bool IsFunctionType() { return kind == TYPE_KIND_FUNCTION; }

		inline u32 SizeOf() { return sizeInBytes; }
		inline u32 AlignOf() { return alignInBytes; }

		inline bool HasFlag(u32 flag) { return (IsBuiltinType() && (basic.flags & flag) != 0); }

		inline bool IsBoolean() { return HasFlag(TYPE_FLAG_BOOLEAN); }
		inline bool IsInteger() { return HasFlag(TYPE_FLAG_INTEGER); }
		inline bool IsSigned() { return HasFlag(TYPE_FLAG_SIGNED); }
		inline bool IsUnsigned() { return HasFlag(TYPE_FLAG_UNSIGNED); }
		inline bool IsFloatingPoint() { return HasFlag(TYPE_FLAG_FLOATING_POINT); }
		inline bool IsString() { return HasFlag(TYPE_FLAG_STRING); }
		inline bool IsArchDependent() { return HasFlag(TYPE_FLAG_ARCH_DEP); }
		inline bool IsArithmetic() { return HasFlag(TYPE_FLAG_INTEGER) || HasFlag(TYPE_FLAG_FLOATING_POINT); }

		bool IsImplicitlyConvertibleTo(Type* other);
		bool IsExplicitlyConvertibleTo(Type* other);

		static Type* GetBuiltinType(BuiltinTypeKind kind);
		static Type* TryGetBuiltinType(BuiltinTypeKind kind);

		static Type* GetImplicitCommonType(Type* a, Type* b);

	public:
		TypeKind kind;

		u32 sizeInBytes;
		u32 alignInBytes;

		union {
			BuiltinType basic;
			FunctionType fn;
		};
	};

} // namespace Wandelt
