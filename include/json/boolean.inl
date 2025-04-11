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

#include <ios>
#include <sstream>
#include <string_view>

namespace js {

template <class T>
   requires(std::is_same_v<T, bool>)
struct Serializer<T> {

   static constexpr std::string_view SkipSpace(std::string_view json) noexcept(false) {
      auto it = json.begin();
      for (; it != json.end(); ++it) {
         if (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r') {
            continue;
         } else {
            break;
         }
      }

      return std::string_view{std::next(it), json.end()};
   }

   static constexpr std::pair<T, std::string_view> Unserialize(std::string_view json) noexcept(false
   ) {
      json = SkipSpace(json);

      if (json.size() < 4) {
         throw ParsingError("Invalid boolean value"_ss, json);
      }

      if (json.size() == 5 && json.substr(0, 5) == "false") {
         return {false, json};
      } else if (json.substr(0, 5) == "true") {
         return {true, json};
      }

      throw ParsingError("Invalid boolean value"_ss, json);
   }

   static constexpr std::string Serialize(T const& elem) noexcept {
      std::stringstream ss;
      ss << std::boolalpha << elem;
      return ss.str();
   }
};

}  // namespace js