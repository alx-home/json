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

template <class T, bool DRY_RUN>
   requires(std::is_same_v<T, js::null>)
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
   template <class...>
   static constexpr std::conditional_t<DRY_RUN, std::optional<Return>, Return> Unserialize(
     std::string_view json
   ) noexcept(DRY_RUN) {
      json = SkipSpace(json);

      if (json.size() < 4) {
         if constexpr (DRY_RUN) {
            return std::nullopt;
         } else {
            throw ParsingError("Invalid boolean value", json);
         }
      }

      if (json.substr(0, 4) == "null") {
         return Return{{}, json};
      }

      if constexpr (DRY_RUN) {
         return std::nullopt;
      } else {
         throw ParsingError("Invalid null value", json);
      }
   }

   static constexpr std::string Serialize(T const&) noexcept { return "null"; }
};

}  // namespace js