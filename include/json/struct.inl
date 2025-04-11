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
#include <tuple>
#include <unordered_set>

namespace js {

template <typename> struct IsMemptr : std::false_type {};
template <typename T> struct IsMemptr<std::pair<std::string, T>> : std::true_type {};

template <class T, class M>
concept is_member_pointer = requires(T t, M m) {
   IsMemptr<M>::value;
   t.*(m.second);
};

template <bool> struct is_true;

template <> struct is_true<true> : std::true_type {};

template <class T>
concept is_reflectable = requires {
   is_tuple<std::remove_cvref_t<decltype(T::PROTOTYPE)>>;
   is_true<([]<std::size_t... N>(std::index_sequence<N...>) consteval {
      return (is_member_pointer<T, decltype(std::get<N>(T::PROTOTYPE))> && ...);
   }(std::make_index_sequence<std::tuple_size_v<decltype(T::PROTOTYPE)>>()))>::value;
};

template <class T>
   requires(is_reflectable<T> && std::is_default_constructible_v<T>)
struct Serializer<T> {
   enum SearchType { OPENING, CLOSING, NEXT, SEP };

   enum Mode { TRY, EXCEPT };
   template <SearchType SEARCH_TYPE, Mode MODE = EXCEPT>
   static constexpr auto Find(std::string_view json) noexcept(false) {
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

            if constexpr (MODE == EXCEPT) {
               throw ParsingError("Unexpected char : "_ss << *it, json);
            } else {
               return std::optional<std::string_view>{};
            }
         }
      }

      if (it == json.end()) {
         if constexpr (MODE == EXCEPT) {
            if constexpr (SEARCH_TYPE == OPENING) {
               throw ParsingError("Opening brace not found", json);
            } else if constexpr (SEARCH_TYPE == NEXT || SEARCH_TYPE == CLOSING) {
               throw ParsingError("Closing brace not found", json);
            } else if constexpr (SEARCH_TYPE == SEP) {
               throw ParsingError("Key/Prop separator not found", json);
            }
         } else {
            return std::optional<std::string_view>{};
         }
      }
      if constexpr (MODE == EXCEPT) {
         return std::string_view{std::next(it), json.end()};
      } else {
         return std::optional<std::string_view>{{std::next(it), json.end()}};
      }
   }

   static constexpr std::pair<T, std::string_view> Unserialize(std::string_view json) noexcept(false
   ) {
      json = Find<OPENING>(json);

      T                               result{};
      std::unordered_set<std::string> keys{};

      while (true) {
         {
            auto result = Find<CLOSING, TRY>(json);
            if (result.has_value()) {
               json = result.value();
               break;
            }
         }

         if (keys.size()) {
            json = Find<NEXT>(json);
         }

         std::string key;
         std::tie(key, json) = Serializer<std::string>::Unserialize(json);
         if (!keys.emplace(key).second) {
            throw ParsingError("Multiple value for key : "_ss << key, json);
         }

         json = Find<SEP>(json);

         bool found = false;
         std::apply(
            [&](auto const&... members) constexpr {
               (
                  [&](auto const& member) constexpr {
                     if (member.first == key) {
                        if (found) {
                           throw std::runtime_error("We shall not be there...");
                        }

                        found = true;

                        std::tie(result.*member.second, json) = Serializer<
                           std::remove_cvref_t<decltype(result.*member.second)>>::Unserialize(json);
                     }
                  }(members),
                  ...
               );
            },
            T::PROTOTYPE
         );

         if (!found) {
            throw ParsingError("Unexpected object key : "_ss << key, json);
         }
      }

      if (keys.size() != std::tuple_size_v<decltype(T::PROTOTYPE)>) {
         throw ParsingError("Missing object key", json);
      }

      return {result, std::string_view{json.begin(), json.end()}};
   }

   static constexpr std::string Serialize(T const& elem) noexcept {
      std::string result = "{";

      std::apply(
         [&](auto const&... members) {
            (
               [&](auto const& member) {
                  if (result.size() > 1) {
                     result += ",";
                  }

                  result += "\"" + member.first + "\":" + Serialize(elem.*member.second);
               }(members),
               ...
            );
         },
         T::PROTOTYPE
      );

      return result + "}";
   }
};

}  // namespace js