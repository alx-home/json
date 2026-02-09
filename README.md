[![Build](https://github.com/alx-home/json/actions/workflows/build.yml/badge.svg)](https://github.com/alx-home/json/actions/workflows/build.yml)

# alx-home json

Typed JSON parser and serializer for C++20 with compile-time reflection.
Define a struct prototype, then `js::Parse` and `js::Stringify` do the rest.

## Features

- Parse and stringify structs with `PROTOTYPE` member mappings.
- Supports `std::string`, numbers, `bool`, `js::null`.
- Containers: `std::vector`, `std::map` (string keys), tuples/arrays.
- Optional fields with `std::optional`.
- `std::variant` support for union types.
- String enums with `js::Enum<"a", "b">`.
- Pretty printing with configurable indentation.
- `js::Pparse` returns the remaining input after a value.

## Requirements

- C++20 compiler.
- CMake 3.20+.
- Dependency: `alx-home::cpp_utils` (linked by CMake).

## Basic usage

```cpp
#include <json/json.h>
#include <optional>
#include <string>
#include <vector>

struct Airport {
	 std::string icao_;
	 std::string name_;
	 std::optional<double> elevation_ft_;

	 static constexpr js::Proto PROTOTYPE{
		 js::_{"icao", &Airport::icao_},
		 js::_{"name", &Airport::name_},
		 js::_{"elevation_ft", &Airport::elevation_ft_},
	 };
};

auto airport = js::Parse<Airport>(R"({"icao":"LFPG","name":"CDG","elevation_ft":392})");
auto json = js::Stringify(airport, true); // pretty print
```

## Variants and enums

```cpp
#include <json/json.h>
#include <variant>

using Mode = js::Enum<"auto", "manual">;
using Payload = std::variant<int, std::string>;

struct Config {
	 Mode mode_{"auto"};
	 Payload payload_{42};

	 static constexpr js::Proto PROTOTYPE{
		 js::_{"mode", &Config::mode_},
		 js::_{"payload", &Config::payload_},
	 };
};

auto cfg = js::Parse<Config>(R"({"mode":"manual","payload":"hello"})");
```

## Prototype composition

Use `js::Extend` to build derived prototypes from base types.

```cpp
#include <json/json.h>
#include <string>

struct Base {
	 std::string id_;

	 static constexpr js::Proto PROTOTYPE{
		 js::_{"id", &Base::id_},
	 };
};

struct Derived : Base {
	 int value_{0};

	 static constexpr js::Proto PROTOTYPE{js::Extend{
		 Base::PROTOTYPE,
		 js::_{"value", &Derived::value_},
	 }};
};
```

## Pretty printing

`js::Stringify` supports optional indentation. The first template parameter is
the indent size, the second selects spaces (true) or tabs (false).

```cpp
auto compact = js::Stringify(value);
auto pretty = js::Stringify<2, true>(value, true);
```

## Partial parsing

`js::Pparse` parses a value and returns the remaining input so you can parse
streamed or concatenated values.

```cpp
auto [first, rest] = js::Pparse<int>("1 2 3");
auto [second, rest2] = js::Pparse<int>(rest);
```

## Errors

Parsing throws `js::ParsingError` with a message and location when input is invalid.
