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

#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

namespace js {

template <class T>
concept is_resizable = requires(T a) { a.emplace_back(std::declval<typename T::value_type>()); };

template <class T, bool DRY_RUN>
   requires(is_resizable<T> && !std::is_same_v<T, std::string>)
struct Serializer<T, DRY_RUN> {
   enum SearchType { OPENING, CLOSING, NEXT };
   template <SearchType SEARCH_TYPE>
   static constexpr std::conditional_t<DRY_RUN, std::optional<std::string_view>, std::string_view>
   Find(std::string_view json) noexcept(DRY_RUN) {
      auto it = json.begin();
      for (; it != json.end(); ++it) {
         if (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r') {
            continue;
         } else {
            if constexpr (SEARCH_TYPE == OPENING) {
               if (*it == '[') {
                  break;
               }
            } else if constexpr (SEARCH_TYPE == CLOSING) {
               if (*it == ']') {
                  break;
               }
            } else {
               if (*it == ',') {
                  break;
               }
            }

            if constexpr (DRY_RUN) {
               return std::nullopt;
            } else {
               throw ParsingError(std::format("Unexpected char : \"{}\"", *it), json);
            }
         }
      }

      if (it == json.end()) {
         if constexpr (DRY_RUN) {
            return std::nullopt;
         } else if constexpr (SEARCH_TYPE == OPENING) {
            throw ParsingError("Opening bracket not found", json);
         } else {
            throw ParsingError("Closing bracket not found", json);
         }
      }

      return std::string_view{std::next(it), json.end()};
   }

   using Return = std::pair<T, std::string_view>;
   template <class...>
   static constexpr std::conditional_t<DRY_RUN, std::optional<Return>, Return> Parse(
     std::string_view json
   ) noexcept(DRY_RUN) {
      if constexpr (DRY_RUN) {
         if (auto result = Find<OPENING>(json); result) {
            json = *result;
         } else {
            return std::nullopt;
         }
      } else {
         json = Find<OPENING>(json);
      }

      T           result{};
      std::size_t index = 0;

      while (true) {
         if (auto result = Serializer<T, true>::template Find<Serializer<T, true>::CLOSING>(json);
             result) {
            json = *result;
            break;
         }

         if (index) {
            if constexpr (DRY_RUN) {
               if (auto result = Find<NEXT>(json); result) {
                  json = *result;
               } else {
                  return std::nullopt;
               }
            } else {
               json = Find<NEXT>(json);
            }
         }

         if constexpr (DRY_RUN) {
            if (auto opt_result = Serializer<typename T::value_type, DRY_RUN>::Parse(json);
                opt_result) {
               auto& [value, next] = *opt_result;
               json                = next;

               result.emplace_back(std::move(value));
               ++index;
            } else {
               return std::nullopt;
            }
         } else {
            auto const& [value, next] = Serializer<typename T::value_type, DRY_RUN>::Parse(json);
            json                      = next;

            result.emplace_back(std::move(value));
            ++index;
         }
      }

      return Return{result, json};
   }

   template <std::size_t INDENT_SIZE, bool INDENT_SPACE>
   static constexpr std::string
   Stringify(T const& elem, std::optional<std::size_t> indent) noexcept(false) {
      auto const [indent_str, next_indent, next_indent_size] =
        NextIndent<INDENT_SIZE, INDENT_SPACE, true>(indent);

      std::string result = "[";
      for (auto const& value : elem) {
         if (result.size() > 1) {
            result += ",";
         }

         result +=
           next_indent
           + Serializer<typename T::value_type>::template Stringify<INDENT_SIZE, INDENT_SPACE>(
             value, next_indent_size
           );
      }

      return result + (result.size() > 1 ? indent_str + "]" : "]");
   }
};
}  // namespace js