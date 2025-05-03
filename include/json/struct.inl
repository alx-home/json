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
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace js {

template <typename>
struct IsMemptr : std::false_type {};
template <typename T>
struct IsMemptr<std::pair<std::string, T>> : std::true_type {};

template <class T, class M>
concept is_member_pointer = requires(T t, M m) {
   IsMemptr<M>::value;
   t.*(m.second);
};

template <bool>
struct is_true;

template <>
struct is_true<true> : std::true_type {};

template <class T>
concept is_reflectable = requires {
   is_tuple<std::remove_cvref_t<decltype(T::PROTOTYPE)>>;
   is_true<([]<std::size_t... N>(std::index_sequence<N...>) consteval {
      return (is_member_pointer<T, decltype(std::get<N>(T::PROTOTYPE))> && ...);
   }(std::make_index_sequence<std::tuple_size_v<decltype(T::PROTOTYPE)>>()))>::value;
};

template <class T, bool DRY_RUN>
   requires(is_reflectable<T> && std::is_default_constructible_v<T>)
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
   static constexpr std::conditional_t<DRY_RUN, std::optional<Return>, Return> Unserialize(
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

      T                               result{};
      std::unordered_set<std::string> keys{};
      std::unordered_set<std::string> required_keys{};
      std::apply(
        [&](auto const&... members) {
           (
             [&](auto const& member) {
                if constexpr (!IS_OPTIONAL<decltype(std::declval<T>().*member.second)>) {
                   required_keys.emplace(member.first);
                }
             }(members),
             ...
           );
        },
        T::PROTOTYPE
      );

      while (true) {
         if (auto result = Serializer<T, true>::template Find<Serializer<T, true>::CLOSING>(json);
             result) {
            json = *result;
            break;
         }

         if (keys.size()) {
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
            if (auto result = Serializer<std::string, DRY_RUN>::Unserialize(json); result) {
               std::tie(key, json) = *result;
            } else {
               return std::nullopt;
            }
         } else {
            std::tie(key, json) = Serializer<std::string, DRY_RUN>::Unserialize(json);
         }

         if (!keys.emplace(key).second) {
            if constexpr (DRY_RUN) {
               return std::nullopt;
            } else {
               throw ParsingError(std::format("Multiple value for key : \"{}\"", key), json);
            }
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

         bool found = false;
         bool error = false;
         std::apply(
           [&](auto const&... members) constexpr {
              (
                [&](auto const& member) constexpr {
                   if constexpr (DRY_RUN) {
                      if (error) {
                         return;
                      }
                   }

                   if (member.first == key) {
                      if (found) {
                         throw std::runtime_error("We shall not be there...");
                      }

                      found = true;

                      using ElemType = std::remove_cvref_t<decltype(result.*member.second)>;

                      if constexpr (DRY_RUN) {
                         if (auto opt_result = Serializer<ElemType, DRY_RUN>::Unserialize(json);
                             opt_result) {
                            std::tie(result.*member.second, json) = *opt_result;
                         } else {
                            error = true;
                         }
                      } else {
                         std::tie(result.*member.second, json) =
                           Serializer<ElemType, DRY_RUN>::Unserialize(json);
                      }
                   }
                }(members),
                ...
              );
           },
           T::PROTOTYPE
         );

         if constexpr (DRY_RUN) {
            if (error) {
               return std::nullopt;
            }
         }

         if (!found) {
            if constexpr (DRY_RUN) {
               return std::nullopt;
            } else {
               throw ParsingError(std::format("Unexpected object key : \"{}\"", key), json);
            }
         }
      }

      for (auto const& key : required_keys) {
         if (!keys.contains(key)) {
            if constexpr (DRY_RUN) {
               return std::nullopt;
            } else {
               throw ParsingError(std::format("Missing object key : \"{}\"", key), json);
            }
         }
      }

      return Return{result, std::string_view{json.begin(), json.end()}};
   }

   static constexpr std::string Serialize(T const& elem) noexcept(false) {
      std::string result = "{";

      std::apply(
        [&](auto const&... members) {
           (
             [&](auto const& member) {
                using ElemType = std::remove_cvref_t<decltype(elem.*member.second)>;

                if constexpr (IS_OPTIONAL<ElemType>) {
                   if (elem.*member.second) {
                      if (result.size() > 1) {
                         result += ",";
                      }

                      result += "\"" + member.first + "\":"
                                + Serializer<typename ElemType::value_type>::Serialize(
                                  *elem.*member.second
                                );
                   }
                } else {
                   if (result.size() > 1) {
                      result += ",";
                   }

                   result += "\"" + member.first
                             + "\":" + Serializer<ElemType>::Serialize(elem.*member.second);
                }
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