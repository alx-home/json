/*
MIT License

Copyright (c) 2025 alx-home

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once

#include "json.h"

#include "utils/String.h"

#include <optional>
#include <tuple>
#include <type_traits>

namespace js {

template <class T> struct IsTupleT : std::false_type {};

template <class... T> struct IsTupleT<std::tuple<T...>> : std::true_type {};

template <class T>
concept is_tuple = requires { IsTupleT<T>::value; };

template <class T> struct IsOptional : std::false_type {};
template <class T> struct IsOptional<std::optional<T>> : std::true_type {};
template <class T> static constexpr bool IS_OPTIONAL = IsOptional<T>::value;

template <class T> struct OptionalValueHelper {
   using type = T;
};

template <class T>
   requires(IS_OPTIONAL<T>)
struct OptionalValueHelper<T> {
   using type = typename T::value_type;
};

template <class T> using OPTIONAL_VALUE_HELPER = typename OptionalValueHelper<T>::type;

}  // namespace js

template <class TYPE, utils::String... VALUES2>
[[nodiscard]] constexpr auto
operator<=>(js::Enum<VALUES2...> const& lhs, TYPE const& rhs) noexcept {
   return static_cast<std::string_view>(lhs) <=> rhs;
}

template <class TYPE, utils::String... VALUES2>
[[nodiscard]] constexpr auto
operator<=>(TYPE const& lhs, js::Enum<VALUES2...> const& rhs) noexcept {
   return lhs <=> static_cast<std::string_view>(rhs);
}