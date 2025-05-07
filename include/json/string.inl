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
#include <algorithm>
#include <format>
#include <iomanip>
#include <ios>
#include <optional>
#undef min

#include "exceptions.h"
#include "json.h"

#include <iterator>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <cassert>

namespace js {

template <class T, bool DRY_RUN>
   requires(std::is_constructible_v<std::remove_cvref_t<T>, std::string> && std::is_constructible_v<std::string_view, std::remove_cvref_t<T>>)
struct Serializer<T, DRY_RUN> {
   static constexpr std::conditional_t<DRY_RUN, std::optional<std::string_view>, std::string_view>
   FindOpening(std::string_view json, char& quote) noexcept(DRY_RUN) {
      auto it = json.begin();
      for (; it != json.end(); ++it) {
         if (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r') {
            continue;
         } else {
            if (*it == '"' || *it == '\'') {
               quote = *it;
               break;
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
         } else {
            throw ParsingError("Opening quote not found", json);
         }
      }

      return std::string_view{std::next(it), json.end()};
   }

   static constexpr std::conditional_t<DRY_RUN, std::optional<std::string>, std::string>
   UnicodeUtf8(uint32_t unicode, std::string_view json) noexcept(DRY_RUN) {
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
         if constexpr (DRY_RUN) {
            return std::nullopt;
         } else {
            throw ParsingError("Invalid utf8 sequence", json);
         }
      }

      return result;
   }

   using StringReturn = std::pair<std::string_view, std::string>;
   static constexpr std::conditional_t<DRY_RUN, std::optional<StringReturn>, StringReturn>
   ParseString(std::string_view json, char quote) noexcept(DRY_RUN) {
      auto        it = json.begin();
      std::string result{};

      while (it != json.end()) {
         if (*it == '\\') {
            ++it;
            if (it == json.end()) {
               if constexpr (DRY_RUN) {
                  return std::nullopt;
               } else {
                  throw ParsingError("Invalid escape sequence", json);
               }
            }

            if (*it == 'u') {
               ++it;
               std::string unicode{};

               if constexpr (DRY_RUN) {
                  if (auto result = ParseUnicode({it, json.end()}); !result) {
                     return std::nullopt;
                  } else {
                     std::tie(json, unicode) = *result;
                  }
               } else {
                  std::tie(json, unicode) = ParseUnicode({it, json.end()});
               }

               it = json.begin();

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
                  if constexpr (DRY_RUN) {
                     return std::nullopt;
                  } else {
                     throw ParsingError("Invalid escape sequence", json);
                  }
               }

               ++it;
            }
         } else if (*it == quote) {
            return StringReturn{{std::next(it), json.end()}, result};
         } else {
            result += *it;
            ++it;
         }
      }
      if constexpr (DRY_RUN) {
         return std::nullopt;
      } else {
         throw ParsingError("Closing quote not found", json);
      }
   }

   using StringReturn = std::pair<std::string_view, std::string>;
   static constexpr std::conditional_t<DRY_RUN, std::optional<StringReturn>, StringReturn>
   ParseUnicode(std::string_view json) noexcept(DRY_RUN) {
      if (json.size() < 4) {
         if constexpr (DRY_RUN) {
            return std::nullopt;
         } else {
            throw ParsingError("Invalid unicode sequence", json);
         }
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
            if constexpr (DRY_RUN) {
               return std::nullopt;
            } else {
               throw ParsingError("Invalid unicode sequence", json);
            }
         }
      }

      assert(shift == 24);
      if constexpr (DRY_RUN) {
         if (auto result = UnicodeUtf8(code, json); result) {
            return StringReturn{{it, json.end()}, *result};
         } else {
            return std::nullopt;
         }
      } else {
         return {{it, json.end()}, UnicodeUtf8(code, json)};
      }
   }

   using Return = std::pair<T, std::string_view>;
   template <class...>
   static constexpr std::conditional_t<DRY_RUN, std::optional<Return>, Return> Parse(
     std::string_view json
   ) noexcept(DRY_RUN) {
      static_assert(
        !std::is_base_of_v<std::string_view, std::remove_cvref_t<T>>,
        "Can't be a string_view since the resulting string might be transformed"
      );

      char quote = '"';

      if constexpr (DRY_RUN) {
         if (auto result = FindOpening(json, quote); !result) {
            return std::nullopt;
         } else {
            json = *result;
         }
      } else {
         json = FindOpening(json, quote);
      }

      // Can't be a string_view since the resulting string might be transformed during ParseString
      std::string result;

      if constexpr (DRY_RUN) {
         if (auto opt_result = ParseString(json, quote); !opt_result) {
            return std::nullopt;
         } else {
            std::tie(json, result) = *opt_result;
         }
      } else {
         std::tie(json, result) = ParseString(json, quote);
      }

      return Return{result, json};
   }

   static constexpr std::string EscapeString(std::string_view string) noexcept(false) {
      std::stringstream result;

      result << std::setfill('0') << std::uppercase << std::hex;

      for (auto it = string.begin(); it != string.end(); ++it) {
         if (*it == '\\') {
            result << "\\\\";
         } else if (*it == '\b') {
            result << "\\b";
         } else if (*it == '\f') {
            result << "\\f";
         } else if (*it == '\r') {
            result << "\\r";
         } else if (*it == '\n') {
            result << "\\n";
         } else if (*it == '\t') {
            result << "\\t";
         } else if (*it == '"') {
            result << "\\\"";
         } else {
            // if (*it >= 0 && *it <= 0x1f) {
            // result << "\\u" << std::setw(4) << static_cast<int>(*it); @todo
            // } else if ((*it & 0x80) == 0x0) {
            // ascii
            result << *it;
            // } else if ((*it & 0xE0) == 0xC0) {
            // unicode 2-byte

            // if (std::distance(it, string.end()) < 2) {
            //    throw ParsingError("Invalid utf8 sequence", {it, string.end()});
            // }

            // uint16_t code_point = ((it[0] & 0x1F) << 6) | (it[1] & 0x3F);
            // result << "\\u" << std::setw(4) << code_point;
            // std::advance(it, 1);
            //    result << *it;
            // } else if ((*it & 0xF0) == 0xE0) {
            //    // unicode 3-byte

            //    if (std::distance(it, string.end()) < 3) {
            //       throw ParsingError("Invalid utf8 sequence", {it, string.end()});
            //    }

            //    uint16_t code_point = ((it[0] & 0x0F) << 12) | ((it[1] & 0x3F) << 6) | (it[2] &
            //    0x3F); result << "\\u" << std::setw(4) << code_point; std::advance(it, 2);
            // } else if ((*it & 0xF8) == 0xF0) {
            //    // unicode 4-byte

            //    if (std::distance(it, string.end()) < 4) {
            //       throw ParsingError("Invalid utf8 sequence", {it, string.end()});
            //    }

            //    uint16_t code_point = ((it[0] & 0x07) << 18) | ((it[1] & 0x3F) << 12)
            //                          | ((it[2] & 0x3F) << 6) | (it[3] & 0x3F);
            //    result << "\\u" << std::setw(4) << code_point;
            //    std::advance(it, 3);
            // } else {
            //    assert(false);
            //    throw ParsingError("Invalid utf8 sequence", {it, string.end()});
         }
      }

      return result.str();
   }

   template <std::size_t INDENT_SIZE, bool INDENT_SPACE>
   static constexpr std::string Stringify(T const& elem, std::optional<std::size_t>) noexcept(false
   ) {
      return "\"" + Serializer<T>::EscapeString(elem) + "\"";
   }
};

}  // namespace js