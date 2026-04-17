# Coding Standards

## Type Aliases

Use the type aliases defined in `Core/Defines.hpp` instead of raw C++ types.
Do not use `int`, `unsigned`, `std::uint32_t`, etc. directly.

| Alias   | Underlying type      |
|---------|----------------------|
| `u8`    | `std::uint8_t`       |
| `u16`   | `std::uint16_t`      |
| `u32`   | `std::uint32_t`      |
| `u64`   | `std::uint64_t`      |
| `i8`    | `std::int8_t`        |
| `i16`   | `std::int16_t`       |
| `i32`   | `std::int32_t`       |
| `i64`   | `std::int64_t`       |
| `f32`   | `float`              |
| `f64`   | `double`             |
| `usize` | `std::size_t`        |
| `isize` | `std::ptrdiff_t`     |

## File Creation

Every new file must follow this template structure.

### Header files (`.hpp`)

```cpp
#pragma once

namespace Wandelt
{

}
```

### Source files (`.cpp`)

```cpp
#include "Filename.hpp"

namespace Wandelt
{

}
```

## Class Access Specifiers

Each distinct group of members must have its own explicit `public:`, `protected:`, or `private:` specifier. Do not combine unrelated member groups under a single specifier.

Groups (in order):
1. Constructors / destructors / copy & move operators
2. Getters / setters
3. Public API methods
4. Protected methods
5. Private methods
6. Member variables

**Wrong:**

```cpp
class Foo
{
public:
    Foo() = default;
    ~Foo() = default;

    i32 GetValue() const { return m_Value; }

    void DoSomething();

private:
    i32 m_Value = 0;
};
```

**Correct:**

```cpp
class Foo
{
public:
    Foo() = default;
    ~Foo() = default;

public:
    i32 GetValue() const { return m_Value; }

public:
    void DoSomething();

private:
    i32 m_Value = 0;
};
```

## Naming Conventions

| Element                  | Style             | Example                    |
|--------------------------|-------------------|----------------------------|
| Classes / structs        | PascalCase        | `Application`, `MeshData`  |
| Methods / functions      | PascalCase        | `Init()`, `GetValue()`     |
| Local variables / params | camelCase         | `u32 someVariable = 0;`    |
| Private members          | `m_` + PascalCase | `m_Value`, `m_IsRunning`   |
| Public members           | camelCase         | `isResizeable`             |
| Static member variables  | `s_` + PascalCase | `s_Instance`               |
| Global variables         | `g_` + PascalCase | `g_Config`                 |
| Namespaces               | PascalCase        | `Wandelt`                  |
| Enums / enum values      | PascalCase        | `RenderMode::Forward`      |

## `auto` Usage

Never use `auto` unless its required by the language (e.g. structured bindings)
