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

#include <string_view>
#include <type_traits>
#include <utility>

namespace js {

template <class T>
concept is_resizable = requires(T a) { a.emplace_back(std::declval<typename T::value_type>()); };

template <class T>
   requires(is_resizable<T> && !std::is_same_v<T, std::string>)
struct Serializer<T> {
   enum SearchType { OPENING, CLOSING, NEXT };
   template <SearchType SEARCH_TYPE>
   static constexpr std::string_view Find(std::string_view json) noexcept(false) {
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
            throw ParsingError("Unexpected char : "_ss << *it, json);
         }
      }

      if (it == json.end()) {
         if constexpr (SEARCH_TYPE == OPENING) {
            throw ParsingError("Opening bracket not found", json);
         } else {
            throw ParsingError("Closing bracket not found", json);
         }
      }

      return std::string_view{std::next(it), json.end()};
   }

   static constexpr std::pair<T, std::string_view> Unserialize(std::string_view json) noexcept(false
   ) {
      json = Find<OPENING>(json);

      T           result{};
      std::size_t index = 0;

      while (true) {
         try {
            json = Find<CLOSING>(json);
            break;
         } catch (ParsingError const&) {
         }

         if (index) {
            json = Find<NEXT>(json);
         }

         auto [value, next] = Serializer<typename T::value_type>::Unserialize(json);
         json               = next;

         result.emplace_back(std::move(value));
         ++index;
      }

      return {result, json};
   }

   static constexpr std::string Serialize(T const& elem) noexcept {
      std::string result = "[";
      for (auto const& value : elem) {
         if (result.size() == 1) {
            result += Serialize(value);
         } else {
            result += "," + Serialize(value);
         }
      }

      return result + "]";
   }
};
}  // namespace js