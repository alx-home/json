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

#include "exceptions.h"

#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

std::stringstream
operator""_ss(char const* str, unsigned long long);

namespace js {

template <class U, class V>
using _ = std::pair<U, V>;

template <class T>
struct is_proto_def : std::false_type {};

template <class U, class V>
struct is_proto_def<_<U, V>> : std::true_type {};

template <class... T>
   requires(is_proto_def<T>::value && ...)
using Proto = std::tuple<T...>;

class Serializable {
   virtual ~Serializable() = default;

   virtual std::string   serialize()   = 0;
   virtual Serializable* unserialize() = 0;
};

template <class T>
struct Serializer;

}  // namespace js

#include "json.inl"
#include "number.inl"
#include "boolean.inl"
#include "string.inl"
#include "array.inl"
#include "vector.inl"
#include "struct.inl"

namespace js {

using empty = nullptr_t;

template <class Type>
static constexpr Type
parse(std::string_view json) noexcept(false) {
   if constexpr (std::is_same_v<Type, empty>) {
      return nullptr;
   } else {
      return Serializer<Type>::unserialize(json).first;
   }
}

template <class Type>
static constexpr std::tuple<Type, std::string_view>
pparse(std::string_view json) noexcept(false) {
   if constexpr (std::is_same_v<Type, empty>) {
      return {nullptr, json};
   } else {
      return Serializer<Type>::unserialize(json);
   }
}

template <class Type>
static constexpr std::string
serialize(Type const& elem) noexcept(false) {
   return Serializer<Type>::serialize(elem);
}

}  // namespace js