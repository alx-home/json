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

#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>

namespace js {

template <class T>
struct IsVariant : std::false_type {};
template <class T1, class T2>
struct IsVariant<std::variant<T1, T2>> : std::true_type {};
template <class T>
static constexpr bool IS_VARIANT = IsVariant<T>::value;

template <class T, bool DRY_RUN>
   requires(IS_VARIANT<T>)
struct Serializer<T, DRY_RUN> {
   using Return = std::pair<T, std::string_view>;
   template <class...>
   static constexpr std::conditional_t<DRY_RUN, std::optional<Return>, Return> Parse(
     std::string_view json
   ) noexcept(DRY_RUN) {
      T    result{};
      bool found = false;
      [&json, &found]<class... TYPES>(std::variant<TYPES...>& result) constexpr {
         (([&json, &found, &result]<class TYPE>() constexpr {
             if (found) {
                return;
             }

             if (auto opt_result = Serializer<TYPE, true>::Parse(json); opt_result) {
                found                  = true;
                std::tie(result, json) = *opt_result;
             }
          }.template operator()<TYPES>()),
          ...);
      }(result);

      if (!found) {
         if constexpr (DRY_RUN) {
            return std::nullopt;
         } else {
            throw ParsingError("Invalid variant value", json);
         }
      }

      return {result, json};
   }

   template <std::size_t INDENT_SIZE, bool INDENT_SPACE>
   static constexpr std::string
   Stringify(T const& elem, std::optional<std::size_t> indent) noexcept {
      bool found = false;
      return [&found, &indent]<class... TYPES>(std::variant<TYPES...> const& elem) constexpr {
         return (
           ([&found, &elem, &indent]<class TYPE>() constexpr -> std::string {
              if (!found && std::holds_alternative<TYPE>(elem)) {
                 found = true;
                 return Serializer<TYPE>::template Stringify<INDENT_SIZE, INDENT_SPACE>(
                   std::get<TYPE>(elem), indent
                 );
              }
              return "";
           }.template operator()<TYPES>())
           + ...
         );
      }(elem);
   }
};

}  // namespace js