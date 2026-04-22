# Variable declarations
A variable declaration declares a new variable for the current scope.

```c
int x = 12;
usz y = 4;
```

Variables must be initialized. Not initializing a variable is an error.

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

# Packages
Wandelt programs consist of packages. A package is the unit of compilation and the
top-level namespace for the declarations it contains.

Every Wandelt source file must begin with exactly one package declaration:

```c
package example;
```

The package name is an identifier. All source files that share the same package
name belong to the same package and see each other's top-level declarations
without the need to import anything.

## Source file discovery

The compiler is invoked with a single entry file:

```
wandelt main.wdt
```

Starting from the directory containing that file, the compiler recursively
scans every subdirectory and picks up all files with the `.wdt` extension as
part of the compilation. There is no explicit list of source files and no
build manifest: the directory tree *is* the project. Files belonging to
different packages may live side by side or in any subdirectory; what ties a
file to a package is its `package` declaration, not its location on disk.

## The `#entrypoint` directive

Exactly one package in a program must be marked as the entrypoint. The
entrypoint package is the one whose top-level code runs when the program starts.
A package is marked as the entrypoint with the `#entrypoint` directive, placed
between the package name and the terminating `;`:

```c
package example #entrypoint;
```

It is an error for a program to contain zero entrypoint packages or more than
one. Non-entrypoint packages omit the directive:

```c
package math;
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

Wandelt has two cast forms:

* `cast(T)v` performs a value conversion to the type `T`
* `cast!!(T)v` performs a force cast that reinterprets the bits of `v` as type `T`

Assignments between values of different types require an explicit conversion unless one of the implicit conversions listed below applies.

## Literal typing

Literal typing is context-sensitive.

### Boolean literals

`true` and `false` have type `bool`.

### Floating-point literals

Floating-point literals with an `f` suffix have type `float`.

Floating-point literals with a `d` suffix have type `double`.

### Integer literals

Unsuffixed integer literals are analyzed as follows:

* If there is an expected type and that type can represent the literal value exactly, the literal takes that type directly.
* Otherwise, if the value fits in `int`, the literal has type `int`.
* Otherwise, the literal remains an abstract integer constant until a surrounding context resolves it.

The abstract integer constant type is an internal compile-time-only concept. It is not a source-level type and cannot be written in user code.

Examples:

```c
float x = 12;               // `12` is typed as `float`
double y = 12;              // `12` is typed as `double`
int z = 12;                 // `12` is typed as `int`
long a = 5000000000;        // `5000000000` is typed as `long`
ulong b = 18446744073709551615; // typed as `ulong`
```

### Implicit conversions

Implicit conversions are only allowed for widening conversions within the same numeric family:

* `char` -> `short` -> `int` -> `long`
* `uchar` -> `ushort` -> `uint` -> `ulong`
* `float` -> `double`

No other implicit conversions are allowed.

> [!NOTE]
> The integer-literal rules above are about how literals acquire a type. They do not imply a general implicit conversion from integers to floating-point types.

In particular, there are no implicit conversions:

* between signed and unsigned integer types
* between integers and booleans
* between floating point and integer types
* between `sz`, `usz`, `intptr`, `uintptr` and any other integer type
* between `string` and `cstring`
* to or from `rawptr`

### Explicit value conversions

`cast(T)v` performs a value conversion. It does not reinterpret bits.

Explicit conversions are allowed between all arithmetic types:

* booleans
* integers
* floating point types

This includes:

* widening and narrowing conversions
* signed and unsigned integer conversions
* integer and floating point conversions
* boolean and numeric conversions

There are no normal `cast(T)` conversions involving `string`, `cstring`, or `rawptr` except to the same type.

### Force casts

`cast!!(T)v` reinterprets the bit pattern of `v` as `T`.

## Built-in constants

* `true` and `false` are the boolean constants of type `bool`.

# Functions

A function declaration introduces a named callable with a fixed return type and a body. Functions are declared with the `fn` keyword, followed by the return type, the function name, a parameter list in parentheses, and a body enclosed in braces:

```c
fn int main()
{
    int x = 12;
    usz y = 15;

    return y;
}
```

The body is a sequence of statements. A function with a non-`void` return type must end its execution with a `return` statement that yields a value of the declared return type.

## Calling functions

A function is called by writing its name followed by an argument list in parentheses. A call may appear as an expression or as a top-level statement in the entrypoint package:

```c
return main();
```

## Discarding return values

When a call appears as an expression statement and the called function has a non-`void` return type, the returned value must be used or the call must be prefixed with the `discard` keyword. Silently dropping a non-`void` return value is an error.

```c
fn int work() { return 1; }
fn void log() {}

fn void main()
{
    work();          // error: return value of call to 'work' is unused
    discard work();  // ok: the return value is explicitly ignored
    log();           // ok: 'log' returns 'void'
    discard log();   // warning: redundant 'discard' on a call that returns 'void'
}
```

`discard` is a statement-level prefix; it may only appear immediately before an expression statement and is not an operator that participates in larger expressions.
