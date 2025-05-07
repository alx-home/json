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

#include <utils/String.h>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace js {
template <utils::String... VALUES>
   requires(sizeof...(VALUES) > 0)
struct Enum;
}

template <class TYPE, utils::String... VALUES>
[[nodiscard]]
constexpr auto operator<=>(js::Enum<VALUES...> const& lhs, TYPE const& rhs) noexcept;

template <class TYPE, utils::String... VALUES>
[[nodiscard]]
constexpr auto operator<=>(TYPE const& lhs, js::Enum<VALUES...> const& rhs) noexcept;

// @fixme 3-way comparision operator not used for string literals...

namespace js {

template <class U, class V>
using _ = std::pair<U, V>;

template <class T>
struct IsProtoDef : std::false_type {};

template <class U, class V>
struct IsProtoDef<_<U, V>> : std::true_type {};

template <class... T>
   requires(IsProtoDef<T>::value && ...)
using Proto = std::tuple<T...>;

class Serializable {
   virtual ~Serializable() = default;

   virtual std::string   Stringify() = 0;
   virtual Serializable* Parse()     = 0;
};

template <class T, bool DRY_RUN = false>
struct Serializer;

using null = nullptr_t;

template <utils::String STRING>
struct Cst {
   static constexpr auto VALUE = STRING;

   std::string_view operator*() const { return VALUE.value_.data(); }
};

template <utils::String... VALUES>
   requires(sizeof...(VALUES) > 0)
struct Enum : std::variant<js::Cst<VALUES>...> {
   Enum(char const* value)
      : Enum(std::string_view{value}) {}

   Enum(std::string_view value)
      : std::variant<js::Cst<VALUES>...>{[&value]() constexpr {
         std::variant<js::Cst<VALUES>...> result;

         bool found = false;
         (
           [&result, &value, &found]<utils::String VALUE>() constexpr {
              if (found) {
                 return;
              }
              if (value == std::string_view{VALUE.value_.data()}) {
                 result = js::Cst<VALUE>{};
                 found  = true;
              }
           }.template operator()<VALUES>(),
           ...
         );

         if (!found) {
            throw std::bad_variant_access();
         }
         return result;
      }()} {}

   explicit(false) operator std::string_view() const {
      std::string_view result{};
      (
        [this, &result]<utils::String VALUE>() constexpr {
           if (std::holds_alternative<js::Cst<VALUE>>(*this)) {
              result = *std::get<js::Cst<VALUE>>(*this);
           }
        }.template operator()<VALUES>(),
        ...
      );
      return result;
   }
};

}  // namespace js

#include "json.inl"

#include "number.inl"
#include "boolean.inl"
#include "string.inl"
#include "array.inl"
#include "vector.inl"
#include "struct.inl"
#include "variant.inl"
#include "constant.inl"

namespace js {

template <class TYPE>
static constexpr TYPE
Parse(std::string_view json) noexcept(false) {
   return Serializer<TYPE>::Parse(json).first;
}

template <class TYPE>
static constexpr std::tuple<TYPE, std::string_view>
Pparse(std::string_view json) noexcept(false) {
   return Serializer<TYPE>::Parse(json);
}

template <std::size_t INDENT_SIZE = 3, bool INDENT_SPACE = true>
static constexpr std::string
Stringify(auto const& elem, bool indent = false) noexcept(false) {
   return Serializer<std::remove_cvref_t<decltype(elem)>>::
     template Stringify<INDENT_SIZE, INDENT_SPACE>(
       elem, indent ? std::optional<std::size_t>(0) : std::nullopt
     );
}

}  // namespace js