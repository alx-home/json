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

#include <cassert>
#include <iterator>
#include <sstream>
#include <string_view>
#include <tuple>

namespace js {

template <class T>
   requires(std::is_fundamental_v<T> && !std::is_same_v<bool, T>)
struct Serializer<T> {
   enum SearchType {
      NUMBER,
      NON_NULL_DIGIT,
      ZERO,
      SPACE,
      DOT,
      MINUS,
      MINUS_PLUS,
      EXPONENT
   };
   template <SearchType searchType>

   static constexpr std::pair<std::string_view, std::size_t> skip(std::string_view json) noexcept {
      auto it = json.begin();
      for (; it != json.end(); ++it) {
         if constexpr (searchType == SPACE) {
            if (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r') {
               continue;
            }

            break;
         } else if constexpr (searchType == ZERO) {
            if (*it == '0') {
               return {{std::next(it), json.end()}, 1};
            }

            break;
         } else if constexpr (searchType == NUMBER) {
            if (*it >= '0' && *it <= '9') {
               continue;
            }

            break;
         } else if constexpr (searchType == NON_NULL_DIGIT) {
            if (*it > '0' && *it <= '9') {
               return {{std::next(it), json.end()}, 1};
            }

            break;
         } else if constexpr (searchType == EXPONENT) {
            if (*it == 'E' || *it == 'e') {
               return {{std::next(it), json.end()}, 1};
            }

            break;
         } else if constexpr (searchType == MINUS) {
            if (*it == '-') {
               return {{std::next(it), json.end()}, 1};
            }

            break;
         } else if constexpr (searchType == MINUS_PLUS) {
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

   static constexpr std::pair<T, std::string_view> unserialize(std::string_view json) noexcept(false) {
      std::size_t delta;
      std::tie(json, delta) = skip<SPACE>(json);

      auto const  begin = json.begin();
      std::size_t size  = 0;

      std::tie(json, size)  = skip<MINUS>(json);
      std::tie(json, delta) = skip<ZERO>(json);

      if (delta) {
         size += delta;
         assert(delta == 1);
      } else {
         std::tie(json, delta) = skip<NON_NULL_DIGIT>(json);
         size += delta;

         if (!delta) {
            throw parsing_error("Number must not start with 0 if not null"_ss, json);
         }
         assert(delta == 1);

         std::tie(json, delta) = skip<NUMBER>(json);
         size += delta;
      }

      std::tie(json, delta) = skip<DOT>(json);
      if (delta) {
         size += delta;
         assert(delta == 1);

         std::tie(json, delta) = skip<NUMBER>(json);
         size += delta;

         if (!delta) {
            throw parsing_error("Missing digit after fraction"_ss, json);
         }
      }

      std::tie(json, delta) = skip<EXPONENT>(json);
      if (delta) {
         size += delta;
         assert(delta == 1);

         std::tie(json, delta) = skip<MINUS_PLUS>(json);
         size += delta;

         std::tie(json, delta) = skip<NUMBER>(json);
         size += delta;

         if (!delta) {
            throw parsing_error("Missing digit after exponent"_ss, json);
         }
      }

      std::string       value{begin, begin + size};
      std::stringstream ss{value};
      T                 result;
      ss >> result;
      return {result, json};
   }

   static constexpr std::string serialize(T const& elem) noexcept {
      std::stringstream ss;
      ss << elem;
      return ss.str();
   }
};

}  // namespace js