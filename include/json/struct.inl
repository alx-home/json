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

template <typename>
struct is_memptr : std::false_type {};
template <typename T>
struct is_memptr<std::pair<std::string, T>> : std::true_type {};

template <class T, class M>
concept is_member_pointer = requires(T t, M m) {
   is_memptr<M>::value;
   t.*(m.second);
};

template <bool>
struct is_true;

template <>
struct is_true<true> : std::true_type {};

template <class T>
concept is_reflectable = requires {
   is_tuple<std::remove_cvref_t<decltype(T::prototype)>>;
   is_true<([]<std::size_t... N>(std::index_sequence<N...>) consteval {
      return (is_member_pointer<T, decltype(std::get<N>(T::prototype))> && ...);
   }(std::make_index_sequence<std::tuple_size_v<decltype(T::prototype)>>()))>::value;
};

template <class T>
   requires(is_reflectable<T> && std::is_default_constructible_v<T>)
struct Serializer<T> {
   enum SearchType {
      OPENING,
      CLOSING,
      NEXT,
      SEP
   };

   enum Mode {
      TRY,
      EXCEPT
   };
   template <SearchType searchType, Mode mode = EXCEPT>
   static constexpr auto find(std::string_view json) noexcept(false) {
      auto it = json.begin();
      for (; it != json.end(); ++it) {
         if (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r') {
            continue;
         } else {
            if constexpr (searchType == OPENING) {
               if (*it == '{') {
                  break;
               }
            } else if constexpr (searchType == CLOSING) {
               if (*it == '}') {
                  break;
               }
            } else if constexpr (searchType == NEXT) {
               if (*it == ',') {
                  break;
               }
            } else {
               if (*it == ':') {
                  break;
               }
            }

            if constexpr (mode == EXCEPT) {
               throw parsing_error("Unexpected char : "_ss << *it, json);
            } else {
               return std::optional<std::string_view>{};
            }
         }
      }

      if (it == json.end()) {
         if constexpr (mode == EXCEPT) {
            if constexpr (searchType == OPENING) {
               throw parsing_error("Opening brace not found", json);
            } else if constexpr (searchType == NEXT || searchType == CLOSING) {
               throw parsing_error("Closing brace not found", json);
            } else if constexpr (searchType == SEP) {
               throw parsing_error("Key/Prop separator not found", json);
            }
         } else {
            return std::optional<std::string_view>{};
         }
      }
      if constexpr (mode == EXCEPT) {
         return std::string_view{std::next(it), json.end()};
      } else {
         return std::optional<std::string_view>{{std::next(it), json.end()}};
      }
   }

   static constexpr std::pair<T, std::string_view> unserialize(std::string_view json) noexcept(false) {
      json = find<OPENING>(json);

      T                               result{};
      std::unordered_set<std::string> keys{};

      while (true) {
         {
            auto result = find<CLOSING, TRY>(json);
            if (result.has_value()) {
               json = result.value();
               break;
            }
         }

         if (keys.size()) {
            json = find<NEXT>(json);
         }

         std::string key;
         std::tie(key, json) = Serializer<std::string>::unserialize(json);
         if (!keys.emplace(key).second) {
            throw parsing_error("Multiple value for key : "_ss << key, json);
         }

         json = find<SEP>(json);

         bool found = false;
         std::apply([&](auto const&... members) constexpr {
            ([&](auto const& member) constexpr {
               if (member.first == key) {
                  if (found) {
                     throw std::runtime_error("We shall not be there...");
                  }

                  found = true;

                  std::tie(result.*member.second, json) = Serializer<std::remove_cvref_t<decltype(result.*member.second)>>::unserialize(json);
               }
            }(members),
             ...);
         },
                    T::prototype);

         if (!found) {
            throw parsing_error("Unexpected object key : "_ss << key, json);
         }
      }

      if (keys.size() != std::tuple_size_v<decltype(T::prototype)>) {
         throw parsing_error("Missing object key", json);
      }

      return {result, std::string_view{json.begin(), json.end()}};
   }

   static constexpr std::string serialize(T const& elem) noexcept {
      std::string result = "{";

      std::apply([&](auto const&... members) {
         ([&](auto const& member) {
            if (result.size() > 1) {
               result += ",";
            }

            result += "\"" + member.first + "\":" + serialize(elem.*member.second);
         }(members),
          ...);
      },
                 T::prototype);

      return result + "}";
   }
};

}  // namespace js