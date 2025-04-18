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
#undef min

#include "exceptions.h"
#include "json.h"

#include <algorithm>
#include <iterator>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace js {

template <class T>
   requires(std::is_constructible_v<std::remove_cvref_t<T>, std::string_view> && std::is_constructible_v<std::string, std::remove_cvref_t<T>>)
struct Serializer<T> {

   static constexpr std::string_view find_opening(std::string_view json, char& quote) noexcept(false) {
      auto it = json.begin();
      for (; it != json.end(); ++it) {
         if (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r') {
            continue;
         } else {
            if (*it == '"' || *it == '\'') {
               quote = *it;
               break;
            }

            throw parsing_error("Unexpected char : "_ss << *it, json);
         }
      }

      if (it == json.end()) {
         throw parsing_error("Opening quote not found", json);
      }

      return {std::next(it), json.end()};
   }

   static constexpr auto unicode_utf8(uint32_t unicode, std::string_view json) {
      std::string result;

      if (unicode <= 0x7F) {
         result.resize(1);
         result[0] = unicode;
      } else if (unicode <= 0x7FF) {
         result.resize(2);
         result[0] = (0xC0 | (unicode >> 6));
         result[1] = (0x80 | (unicode & 0x3F));
      } else if (unicode <= 0xFFFF) {
         result.resize(3);
         result[0] = (0xE0 | (unicode >> 12));
         result[1] = (0x80 | ((unicode >> 6) & 0x3F));
         result[2] = (0x80 | (unicode & 0x3F));
      } else if (unicode <= 0x10FFFF) {
         result.resize(4);
         result[0] = (0xF0 | (unicode >> 18));
         result[1] = (0x80 | ((unicode >> 12) & 0x3F));
         result[2] = (0x80 | ((unicode >> 6) & 0x3F));
         result[3] = (0x80 | (unicode & 0x3F));
      } else {
         throw parsing_error("Invalid utf8 sequence", json);
      }

      return result;
   }

   static constexpr std::pair<std::string_view, std::string> parse_string(std::string_view json, char quote) {
      auto        it = json.begin();
      std::string result{};

      while (it != json.end()) {
         if (*it == '\\') {
            ++it;
            if (it == json.end()) {
               throw parsing_error("Invalid escape sequence", json);
            }

            if (*it == 'u') {
               ++it;
               std::string unicode{};
               std::tie(unicode, json) = parse_unicode({it, json.end()});
               it                      = json.begin();

               result += unicode;
            } else {
               if (*it == '\\') {
                  result += '\\';
               } else if (*it == 'b') {
                  result += '\b';
               } else if (*it == 'f') {
                  result += '\f';
               } else if (*it == 'r') {
                  result += '\r';
               } else if (*it == 'n') {
                  result += '\n';
               } else if (*it == 't') {
                  result += '\t';
               } else if (*it == '/') {
                  result += '/';
               } else if (*it == '"') {
                  result += '"';
               } else if (*it == '\'') {
                  result += '\'';
               } else {
                  throw parsing_error("Invalid escape sequence", json);
               }

               ++it;
            }
         } else if (*it == quote) {
            return {{std::next(it), json.end()}, result};
         } else {
            result += *it;
            ++it;
         }
      }

      throw parsing_error("Closing quote not found", json);
   }

   static constexpr std::pair<std::string, std::string_view> parse_unicode(std::string_view json) {
      if (json.size() < 4) {
         throw parsing_error("Invalid unicode sequence", json);
      }

      uint32_t    code{0};
      std::size_t shift{0};
      auto const  end = json.begin() + 4;
      auto        it  = json.begin();

      for (; it != end; ++it, shift += 8) {
         if (*it >= '0' && *it <= '9') {
            code <<= shift;
            code |= (*it - '0');
         } else if (*it >= 'a' && *it <= 'f') {
            code <<= shift;
            code |= 0xa + (*it - 'a');
         } else if (*it >= 'A' && *it <= 'F') {
            code <<= shift;
            code |= 0xa + (*it - 'a');
         } else {
            throw parsing_error("Invalid unicode sequence", json);
         }
      }

      assert(shift == 24);
      return {unicode_utf8(code, json), {it, json.end()}};
   }

   static constexpr std::pair<T, std::string_view> unserialize(std::string_view json) noexcept(false) {
      char quote = '"';

      json = find_opening(json, quote);
      std::string result;
      std::tie(json, result) = parse_string(json, quote);

      return std::pair<T, std::string_view>{result, json};
   }

   static constexpr auto escape_string(std::string_view string) {
      std::string result;

      for (auto it = string.begin(); it != string.end(); ++it) {
         if (*it == '\\') {
            result += "\\\\";
         } else if (*it == '\b') {
            result += "\\b";
         } else if (*it == '\f') {
            result += "\\f";
         } else if (*it == '\r') {
            result += "\\r";
         } else if (*it == '\n') {
            result += "\\n";
         } else if (*it == '\t') {
            result += "\\t";
         } else if (*it == '"') {
            result += "\\\"";
         } else if (*it >= 0 && *it <= 0x1f) {
            auto h = *it & 0xF;
            result += std::string{"\\u00"} + (*it & 0x10 ? "1" : "0")
                      + static_cast<char>(h > 9 ? ('a' + (h - 9)) : ('0' + h));
         } else if ((*it & 0x80) == 0x0) {
            // ascii
            result += *it;
         } else if ((*it & 0xE0) == 0xC0) {
            // unicode 2-byte

            if (std::distance(it, string.end()) < 2) {
               throw parsing_error("Invalid utf8 sequence", {it, string.end()});
            }

            uint16_t code_point = ((it[0] & 0x1F) << 6) | (it[1] & 0x3F);
            result += std::string{"\\u00"}
                      + static_cast<char>((code_point >> 8) & 0xFF)
                      + static_cast<char>(code_point & 0xFF);
            it += 2;
         } else if ((*it & 0xF0) == 0xE0) {
            // unicode 3-byte

            if (std::distance(it, string.end()) < 3) {
               throw parsing_error("Invalid utf8 sequence", {it, string.end()});
            }

            uint16_t code_point = ((it[0] & 0x0F) << 12) | ((it[1] & 0x3F) << 6) | (it[2] & 0x3F);
            result += std::string{"\\u0"}
                      + static_cast<char>((code_point >> 16) & 0xFF)
                      + static_cast<char>((code_point >> 8) & 0xFF)
                      + static_cast<char>(code_point & 0xFF);
            it += 3;
         } else if ((*it & 0xF0) == 0xE0) {
            // unicode 4-byte

            if (std::distance(it, string.end()) < 4) {
               throw parsing_error("Invalid utf8 sequence", {it, string.end()});
            }

            uint16_t code_point = ((it[0] & 0x07) << 18) | ((it[1] & 0x3F) << 12) | ((it[2] & 0x3F) << 6) | (it[3] & 0x3F);
            result += std::string{"\\u"}
                      + static_cast<char>((code_point >> 24) & 0xFF)
                      + static_cast<char>((code_point >> 16) & 0xFF)
                      + static_cast<char>((code_point >> 8) & 0xFF)
                      + static_cast<char>(code_point & 0xFF);
            it += 4;
         } else {
            assert(false);
            throw parsing_error("Invalid utf8 sequence", {it, string.end()});
         }
      }

      return result;
   }

   static constexpr std::string serialize(T const& elem) noexcept {
      return "\"" + escape_string(elem) + "\"";
   }
};

}  // namespace js