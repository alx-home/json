/*
MIT License

Copyright (c) 2025 Alexandre GARCIN

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

#include <cassert>
#include <iomanip>
#include <ios>
#include <iterator>
#include <optional>
#include <sstream>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace js {

template <class T, bool DRY_RUN>
   requires(std::is_fundamental_v<T> && !std::is_same_v<bool, T>)
struct Serializer<T, DRY_RUN> {
   enum SearchType { NUMBER, NON_NULL_DIGIT, ZERO, SPACE, DOT, MINUS, MINUS_PLUS, EXPONENT };
   template <SearchType SEARCH_TYPE>

   static constexpr std::pair<std::string_view, std::size_t> Skip(std::string_view json) noexcept {
      auto it = json.begin();
      for (; it != json.end(); ++it) {
         if constexpr (SEARCH_TYPE == SPACE) {
            if (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r') {
               continue;
            }

            break;
         } else if constexpr (SEARCH_TYPE == ZERO) {
            if (*it == '0') {
               return {{std::next(it), json.end()}, 1};
            }

            break;
         } else if constexpr (SEARCH_TYPE == NUMBER) {
            if (*it >= '0' && *it <= '9') {
               continue;
            }

            break;
         } else if constexpr (SEARCH_TYPE == NON_NULL_DIGIT) {
            if (*it > '0' && *it <= '9') {
               return {{std::next(it), json.end()}, 1};
            }

            break;
         } else if constexpr (SEARCH_TYPE == EXPONENT) {
            if (*it == 'E' || *it == 'e') {
               return {{std::next(it), json.end()}, 1};
            }

            break;
         } else if constexpr (SEARCH_TYPE == MINUS) {
            if (*it == '-') {
               return {{std::next(it), json.end()}, 1};
            }

            break;
         } else if constexpr (SEARCH_TYPE == MINUS_PLUS) {
            if (*it == '-' || *it == '+') {
               return {{std::next(it), json.end()}, 1};
            }

            break;
         } else {
            if (*it == '.') {
               return {{std::next(it), json.end()}, 1};
            }

            break;
         }
      }

      return {{it, json.end()}, std::distance(json.begin(), it)};
   }

   using Return = std::pair<T, std::string_view>;
   template <class...>
   static constexpr std::conditional_t<DRY_RUN, std::optional<Return>, Return> Parse(
     std::string_view json
   ) noexcept(DRY_RUN) {
      std::size_t delta;
      std::tie(json, delta) = Skip<SPACE>(json);

      auto const  begin = json.begin();
      std::size_t size  = 0;

      std::tie(json, size)  = Skip<MINUS>(json);
      std::tie(json, delta) = Skip<ZERO>(json);

      if (delta) {
         size += delta;
         assert(delta == 1);
      } else {
         std::tie(json, delta) = Skip<NON_NULL_DIGIT>(json);
         size += delta;

         if (!delta) {
            if constexpr (DRY_RUN) {
               return std::nullopt;
            } else {
               throw ParsingError("Number must not start with 0 if not null", json);
            }
         }
         assert(delta == 1);

         std::tie(json, delta) = Skip<NUMBER>(json);
         size += delta;
      }

      std::tie(json, delta) = Skip<DOT>(json);
      if (delta) {
         size += delta;
         assert(delta == 1);

         std::tie(json, delta) = Skip<NUMBER>(json);
         size += delta;

         if (!delta) {
            if constexpr (DRY_RUN) {
               return std::nullopt;
            } else {
               throw ParsingError("Missing digit after fraction", json);
            }
         }
      }

      std::tie(json, delta) = Skip<EXPONENT>(json);
      if (delta) {
         size += delta;
         assert(delta == 1);

         std::tie(json, delta) = Skip<MINUS_PLUS>(json);
         size += delta;

         std::tie(json, delta) = Skip<NUMBER>(json);
         size += delta;

         if (!delta) {
            if constexpr (DRY_RUN) {
               return std::nullopt;
            } else {
               throw ParsingError("Missing digit after exponent", json);
            }
         }
      }

      std::string       value{begin, begin + size};
      std::stringstream ss{value};
      T                 result;
      ss >> result;
      return Return{result, json};
   }

   template <std::size_t INDENT_SIZE, bool INDENT_SPACE>
   static constexpr std::string Stringify(T const& elem, std::optional<std::size_t>) noexcept {
      std::stringstream ss;

      if constexpr (std::is_floating_point_v<std::remove_cvref_t<T>>) {
         static constexpr auto MAX_PRECISION{
           std::numeric_limits<std::remove_cvref_t<T>>::digits10 + 1
         };
         ss << std::setprecision(MAX_PRECISION) << std::defaultfloat << elem;
      } else {
         ss << elem;
      }
      return ss.str();
   }
};

}  // namespace js