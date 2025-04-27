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

#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

std::stringstream operator""_ss(char const* str, unsigned long long);

namespace js {

template <class U, class V> using _ = std::pair<U, V>;

template <class T> struct IsProtoDef : std::false_type {};

template <class U, class V> struct IsProtoDef<_<U, V>> : std::true_type {};

template <class... T>
   requires(IsProtoDef<T>::value && ...)
using Proto = std::tuple<T...>;

class Serializable {
   virtual ~Serializable() = default;

   virtual std::string   Serialize()   = 0;
   virtual Serializable* Unserialize() = 0;
};

template <class T, bool DRY_RUN = false> struct Serializer;

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

template <class TYPE>
static constexpr TYPE
Parse(std::string_view json) noexcept(false) {
   if constexpr (std::is_same_v<TYPE, empty>) {
      return nullptr;
   } else {
      return Serializer<TYPE>::Unserialize(json).first;
   }
}

template <class TYPE>
static constexpr std::tuple<TYPE, std::string_view>
Pparse(std::string_view json) noexcept(false) {
   if constexpr (std::is_same_v<TYPE, empty>) {
      return {nullptr, json};
   } else {
      return Serializer<TYPE>::Unserialize(json);
   }
}

template <class TYPE>
static constexpr std::string
Serialize(TYPE const& elem) noexcept(false) {
   return Serializer<TYPE>::Serialize(elem);
}

}  // namespace js