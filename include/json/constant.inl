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

#include "exceptions.h"

#include <string_view>
#include <type_traits>

namespace js {

template <class T> struct IsConstant : std::false_type {};
template <utils::String STRING> struct IsConstant<Cst<STRING>> : std::true_type {};

template <class T> static constexpr bool IS_CONSTANT = IsConstant<T>::value;

template <class T, bool DRY_RUN>
   requires(IS_CONSTANT<T>)
struct Serializer<T, DRY_RUN> {
   static constexpr std::string_view SkipSpace(std::string_view json) noexcept(false) {
      auto it = json.begin();
      for (; it != json.end(); ++it) {
         if (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r') {
            continue;
         } else {
            break;
         }
      }

      return std::string_view{it, json.end()};
   }

   using Return = std::pair<T, std::string_view>;
   static constexpr std::conditional_t<DRY_RUN, std::optional<Return>, Return> Unserialize(
      std::string_view json
   ) noexcept(DRY_RUN) {
      json = SkipSpace(json);

      if (json.empty()) {
         if constexpr (DRY_RUN) {
            return std::nullopt;
         } else {
            throw ParsingError("Unexpected EOF", json);
         }
      }

      char quote = '\"';
      if (json.front() != '\"' || json.front() != '\'') {
         if constexpr (DRY_RUN) {
            return std::nullopt;
         } else {
            throw ParsingError("Opening quote not found", json);
         }

         quote = json.front();
      }

      if (json.starts_with(T::value_.data())) {
         json = json.substr(T::value_.size());

         if (json.empty() || json.front() != quote) {
            if constexpr (DRY_RUN) {
               return std::nullopt;
            } else {
               throw ParsingError(std::format("Closing quote \"{}\" not found", quote), json);
            }
         }

         return Return{{}, json};
      }

      if constexpr (DRY_RUN) {
         return std::nullopt;
      } else {
         throw ParsingError(
            std::format("Invalid constant value (expected: \"{}\")", T::value_.data()), json
         );
      }
   }

   static constexpr std::string Serialize(T const&) noexcept {
      return "\"" + std::string(T::value_.data()) + "\"";
   }
};

}  // namespace js