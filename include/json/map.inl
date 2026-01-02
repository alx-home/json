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

#include "json.inl"

#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

namespace js {

template <class T, bool DRY_RUN>
   requires(is_map<T>)
struct Serializer<T, DRY_RUN> {
   enum SearchType { OPENING, CLOSING, NEXT, SEP };

   template <SearchType SEARCH_TYPE>
   static constexpr std::conditional_t<DRY_RUN, std::optional<std::string_view>, std::string_view>
   Find(std::string_view json) noexcept(DRY_RUN) {
      auto it = json.begin();
      for (; it != json.end(); ++it) {
         if (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r') {
            continue;
         } else {
            if constexpr (SEARCH_TYPE == OPENING) {
               if (*it == '{') {
                  break;
               }
            } else if constexpr (SEARCH_TYPE == CLOSING) {
               if (*it == '}') {
                  break;
               }
            } else if constexpr (SEARCH_TYPE == NEXT) {
               if (*it == ',') {
                  break;
               }
            } else {
               if (*it == ':') {
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
         } else {
            if constexpr (SEARCH_TYPE == OPENING) {
               throw ParsingError("Opening brace not found", json);
            } else if constexpr (SEARCH_TYPE == NEXT || SEARCH_TYPE == CLOSING) {
               throw ParsingError("Closing brace not found", json);
            } else if constexpr (SEARCH_TYPE == SEP) {
               throw ParsingError("Key/Prop separator not found", json);
            }
         }
      }

      return std::string_view{std::next(it), json.end()};
   }

   using Return = std::pair<T, std::string_view>;
   template <class...>
   static constexpr std::conditional_t<DRY_RUN, std::optional<Return>, Return> Parse(
     std::string_view json
   ) noexcept(false) {
      if constexpr (DRY_RUN) {
         if (auto result = Find<OPENING>(json); result) {
            json = *result;
         } else {
            return std::nullopt;
         }
      } else {
         json = Find<OPENING>(json);
      }

      T result{};

      while (true) {
         if (auto result = Serializer<T, true>::template Find<Serializer<T, true>::CLOSING>(json);
             result) {
            json = *result;
            break;
         }

         if (result.size()) {
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

         std::string key;
         if constexpr (DRY_RUN) {
            if (auto result = Serializer<std::string, DRY_RUN>::Parse(json); result) {
               std::tie(key, json) = *result;
            } else {
               return std::nullopt;
            }
         } else {
            std::tie(key, json) = Serializer<std::string, DRY_RUN>::Parse(json);
         }

         if constexpr (DRY_RUN) {
            if (auto result = Find<SEP>(json); result) {
               json = *result;
            } else {
               return std::nullopt;
            }
         } else {
            json = Find<SEP>(json);
         }

         bool error = false;
         if constexpr (DRY_RUN) {
            if (error) {
               return;
            }
         }

         using ElemType = typename T::mapped_type;

         if constexpr (DRY_RUN) {
            if (auto opt_result = Serializer<ElemType, DRY_RUN>::Parse(json); opt_result) {
               json = std::move(opt_result->second);
               result.emplace(std::move(key), std::move(opt_result->first));
            } else {
               error = true;
            }
         } else {
            auto result2 = Serializer<ElemType, DRY_RUN>::Parse(json);
            result.emplace(std::move(key), std::move(result2.first));
            json = std::move(result2.second);
         }

         if constexpr (DRY_RUN) {
            if (error) {
               return std::nullopt;
            }
         }
      }

      return Return{result, std::string_view{json.begin(), json.end()}};
   }

   template <std::size_t INDENT_SIZE, bool INDENT_SPACE>
   static constexpr std::string
   Stringify(T const& elem, std::optional<std::size_t> indent) noexcept(false) {
      auto const [indent_str, next_indent, next_indent_size] =
        NextIndent<INDENT_SIZE, INDENT_SPACE, true>(indent);

      std::string result = "{";
      for (auto const& member : elem) {
         using ElemType = typename T::mapped_type;
         if constexpr (IS_OPTIONAL<ElemType>) {
            if (member.second) {
               if (result.size() > 1) {
                  result += ",";
               }

               result += next_indent + "\"" + std::string{member.first}
                         + "\":" + (indent ? " " : "")
                         + Serializer<typename ElemType::value_type>::
                           template Stringify<INDENT_SIZE, INDENT_SPACE>(
                             *(member.second), next_indent_size
                           );
            }
         } else {
            if (result.size() > 1) {
               result += ",";
            }

            result += next_indent + "\"" + std::string{member.first} + "\":" + (indent ? " " : "")
                      + Serializer<ElemType>::template Stringify<INDENT_SIZE, INDENT_SPACE>(
                        member.second, next_indent_size
                      );
         }
      }
      return result + (result.size() > 1 ? indent_str + "}" : "}");
   }
};

}  // namespace js