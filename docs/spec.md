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

`char` is a signed 8-bit integer type. It is used both as the smallest signed integer type and as the type of character literals.

`sz` and `usz` are signed and unsigned platform-sized types used for indexing and lengths.

> [!NOTE]
> Lengths and indices are signed by default to avoid overflow errors from unsigned arithmetic.

`intptr` and `uintptr` are signed and unsigned pointer-sized integer types, used for storing raw pointer values as integers.

> [!NOTE]
> The `string` type is represented internally as a data pointer plus a length.
> The `cstring` type is represented internally as a pointer to null-terminated character data.

`string` is the language's normal string type. It may contain embedded zero bytes because its length is tracked separately.

`cstring` is intended for C interoperability and other APIs that expect a terminating `\0` byte.

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

### Character literals

Character literals are written in single quotes:

```c
'a'
'\n'
'\\'
'\''
```

Character literals have type `char`.

A character literal must contain exactly one character or one escape sequence.

### String literals

String literals are written in double quotes:

```c
""
"hello"
"line\n"
```

String literals are context-sensitive:

* if the expected type is `cstring`, the literal takes type `cstring`
* otherwise, the literal takes type `string`

The source form may contain escape sequences. The empty string literal `""` is valid.

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

# Expressions

A grouped expression has the same type as the expression inside it.

## Unary negation

Unary `-` negates an arithmetic value:

```c
int x = -12;
float y = -value;
```

The operand must have an arithmetic type. The result has the same type as the operand.

## Binary operators

The following binary operators are currently defined:

* arithmetic: `+`, `-`, `*`, `/`
* equality: `==`, `!=`
* ordering: `<`, `<=`, `>`, `>=`

Arithmetic operators require arithmetic operands. If the operands have different arithmetic types, the compiler first finds a common type both operands can implicitly convert to, then performs the operation in that type.

```c
double x = 1 + 2.5d;
int y = 8 / 2;
```

Equality operators produce a `bool` result. They are valid when both operands already have the same type, or when both operands are arithmetic and can be converted to a common arithmetic type.

Ordering operators produce a `bool` result and require arithmetic operands.

## Operator precedence

From highest to lowest precedence:

* postfix `expr++`, `expr--`, and calls
* prefix unary `-`, `++expr`, `--expr`
* `*`, `/`
* `+`, `-`
* `<`, `<=`, `>`, `>=`
* `==`, `!=`
* assignments

Binary arithmetic, equality, and ordering operators associate left-to-right.

## Grouping

Parentheses may be used to group an expression and override normal operator precedence:

```c
int x = (1 + 2) * 3;
```

## Increment and decrement

Wandelt supports both prefix and postfix increment and decrement:

```c
++x;
y--;
```

The operand must be a variable of arithmetic type. These expressions mutate the operand in place and have the same type as the operand.

## Assignment

Wandelt supports simple and compound assignment:

* `=`
* `+=`, `-=`, `*=`, `/=`

The left-hand side of an assignment must be a variable.

For simple assignment, the right-hand side must have the same type as the target variable or be implicitly convertible to it:

```c
long x = 0;
x = 12;
```

For compound assignment, the operation is checked as an arithmetic update of the left-hand side followed by assignment back to the original variable type.

```c
int x = 10;
x += 2;
x *= 3;
```

An assignment expression has type `void`.

Because assignment has type `void`, chained assignments such as `a = b = c;` are not valid language-level expressions.

# Functions

A function declaration introduces a named callable with a fixed return type and a body. Functions are declared with the `fn` keyword, followed by the return type, the function name, a parameter list in parentheses, and a body enclosed in braces. Each parameter is written as a type followed by a name:

```c
fn int add(int x, int y)
{
    return x;
}
```

Parameter names are part of the function signature at the source level and may be used by named arguments at the call site.

The body is a sequence of statements. A function with a non-`void` return type must end its execution with a `return` statement that yields a value of the declared return type.

## Calling functions

A function is called by writing its name followed by an argument list in parentheses. Arguments may be passed positionally or by parameter name:

```c
return add(12, 15);
return add(y = 15, x = 12);
```

Positional arguments bind by order. Named arguments bind by parameter name and may appear in any order. A single call must use exactly one style: positional or named arguments cannot be mixed.

A call may appear as an expression or as a top-level statement in the entrypoint package.

## Expression statements

Only expressions with a side effect are valid as a statement.

```c
fn int main()
{
    int x = 12;

    x;         // error: expression statement has no effect
    x + 1;     // error: expression statement has no effect
    x = x + 1; // ok: assignment
    x++;       // ok: increment

    return x;
}
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

# Control flow

## If statements

An `if` statement conditionally executes a block based on a boolean condition:

```c
if x == 1 {
    discard work();
}
```

The condition is written directly after the `if` keyword without surrounding parentheses and must have type `bool`. An integer, floating-point, or other non-`bool` condition is an error.

An `if` may be followed by an `else` block, which runs when the condition is false:

```c
if x == 1 {
    discard work();
} else {
    discard other();
}
```

Multiple conditions are chained with `else if`:

```c
if x == 1 {
    discard a();
} else if x == 2 {
    discard b();
} else {
    discard c();
}
```

The `then` block, each `else if` block, and the final `else` block each introduce their own scope. A variable declared inside one branch is not visible after the `if` ends or in sibling branches.

### Return analysis for non-`void` functions

A non-`void` function must return a value on every control path. An `if` chain only guarantees a return when both of the following hold:

* there is a terminating `else` block, and
* every `then` / `else if` / `else` branch itself returns on every control path

An `if` without a terminating `else`, or with any branch that falls through without returning, does not count as a guaranteed return:

```c
fn int f() {
    if cond { return 1; }  // error: missing return on the 'cond is false' path
}

fn int g() {
    if cond { return 1; }
    return 2;              // ok: fallthrough path is covered by the trailing return
}

fn int h() {
    if cond { return 1; } else { return 2; }  // ok: every path returns
}
```

## While loops

A `while` loop repeatedly executes its body as long as a boolean condition holds:

```c
while keep_going {
    discard step();
}
```

The condition is written directly after the `while` keyword without parentheses and must have type `bool`. A non-`bool` condition is an error.

The body is a block and introduces its own scope.

Because a `while` may execute zero times, it does not on its own satisfy the return-on-every-path requirement of a non-`void` function:

```c
fn int f() {
    while cond { return 1; }  // error: loop may not execute
}
```

## For loops

A `for` loop bundles an initializer, a condition, and an increment into a single header:

```c
for int x = 0; x < 10; x++ {
    if x == 5 {
        break;
    }
}
```

The header has three clauses separated by semicolons:

* **init** — a variable declaration or an expression statement. Both forms consume their own trailing `;`. A variable declared here is scoped to the `for` loop and is not visible after it.
* **condition** — an expression of type `bool`, followed by a `;`. A non-`bool` condition is an error.
* **increment** — an expression that must have a side effect (assignment, compound assignment, call, or increment/decrement). A pure expression like `x + 1` is rejected for the same reason a pure expression statement is rejected.

The loop executes the init once, then repeatedly evaluates the condition; for each iteration where the condition is true it runs the body and then the increment.

The init, condition, increment, and body all share a single scope. As with `while`, a `for` loop is not sufficient on its own to satisfy the return-on-every-path requirement.

## Break and continue

`break;` exits the innermost enclosing loop. `continue;` ends the current iteration and proceeds to the next one (for a `for` loop, the increment still runs before the condition is re-checked).

Both statements must appear inside a `while` or `for` loop. A `break` or `continue` outside any loop is an error:

```c
fn void main() {
    break;     // error: 'break' statement is only allowed inside a loop
    continue;  // error: 'continue' statement is only allowed inside a loop
}
```

When loops are nested, `break` and `continue` always apply to the innermost enclosing loop:

```c
while a {
    while b {
        break;  // exits the inner 'while b' loop
    }
    // execution continues here
}
```
