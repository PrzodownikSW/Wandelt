# Comments
Comments can be anywhere outside of a string or character literal. Single line comments begin with //:

```js
// some comment
int x = 12;
```

Multi-line comments begin with <* and end with *>. Multi-line comments can be also be nested:

```js
<*
	some text
	<*
		some text nested
	*>
*>
```

# Basic types
Wandelt’s basic types are:

```js
// booleans
bool

// integers
char  short  int  long  sz  intptr
uchar ushort uint ulong usz uintptr

// floating point numbers
float
double

// strings
string
cstring

// raw pointer type
rawptr
````

`char` is 8-bit, `short` is 16-bit, `int` is 32-bit, `long` is 64-bit. All integer sizes are fixed.

`sz` and `usz` are signed and unsigned platform-sized types used for indexing and lengths.

> [!NOTE]
> Lengths and indices are signed by default to avoid overflow errors from unsigned arithmetic.

`intptr` and `uintptr` are signed and unsigned pointer-sized integer types, used for storing raw pointer values as integers.

> [!NOTE]
> The `string` type stores the pointer to the data and the length of the string. `cstring` is used to interface with foreign libraries written in/for C that use zero-terminated strings.

## Default values

Variables declared without an explicit initial value are given their `default` value.

The default value is:

* 0 for numeric types
* `false` for boolean types
* "" (the empty string) for strings

The `default` keyword might be used to explicitly set initial default value.

## Type conversion

The expression `as(T)v` converts the value `v` to the type `T`.

Assignments between values of a different type require an explicit conversion. Implicit conversions are only allowed for widening casts:

* `char` → `short` → `int` → `long`
* `float` → `double`

All narrowing conversions require an explicit `as(T)` cast. There are no implicit conversions between signed and unsigned types, between integers and booleans, or between floating point and integer types.

The expression `as!!(T)v` is a force cast that reinterprets the bits of `v` as type `T`.

## Built-in constants

* `true` and `false` are the boolean constants of type `bool`.
