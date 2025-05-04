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

#include <format>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace js {

template <class T>
concept is_indexable = requires() { std::tuple_size_v<T>; };

template <bool RESULT, class... T>
struct SparseOptionalHelper;
template <class... T>
struct SparseOptionalHelper<false, T...> : std::false_type {};
template <>
struct SparseOptionalHelper<true> : std::true_type {};

template <class T, class... OTHER>
struct SparseOptionalHelper<true, T, OTHER...>
   : std::bool_constant<
       IS_OPTIONAL<T> ? (IS_OPTIONAL<OTHER>&&...) : SparseOptionalHelper<true, OTHER...>::value> {};

template <class T>
struct SparseOptional;
template <class... T>
struct SparseOptional<std::tuple<T...>>
   : std::bool_constant<SparseOptionalHelper<true, T...>::value> {};

template <class T>
concept not_sparse_optional = requires(T a) {
   { std::tuple_size_v<T> };
   { SparseOptional<T>::value };
};

template <class T, bool DRY_RUN>
   requires(is_indexable<T> && !std::is_same_v<T, std::string> && not_sparse_optional<T>)
struct Serializer<T, DRY_RUN> {
   enum SearchType { OPENING, CLOSING, NEXT };

   template <SearchType SEARCH_TYPE>
   static constexpr std::conditional_t<DRY_RUN, std::optional<std::string_view>, std::string_view>
   Find(std::string_view json) noexcept(DRY_RUN) {
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
         } else if constexpr (SEARCH_TYPE == OPENING) {
            throw ParsingError("Opening bracket not found", json);
         } else {
            throw ParsingError("Closing bracket not found", json);
         }
      }

      return std::string_view{std::next(it), json.end()};
   }

   using Return = std::pair<T, std::string_view>;
   template <class...>
   static constexpr std::conditional_t<DRY_RUN, std::optional<Return>, Return> Unserialize(
     std::string_view json
   ) noexcept(DRY_RUN) {
      if constexpr (DRY_RUN) {
         if (auto result = Find<OPENING>(json); result) {
            json = *result;
         } else {
            return std::nullopt;
         }
      } else {
         json = Find<OPENING>(json);
      }

      bool error  = false;
      bool end    = false;
      auto result = [&]<std::size_t... INDEX>(std::index_sequence<INDEX...>) constexpr {
         return std::tuple{[&]() constexpr {
            using ElemType = std::tuple_element_t<INDEX, T>;
            using Return   = std::conditional_t<DRY_RUN, std::optional<ElemType>, ElemType>;
            if constexpr (IS_OPTIONAL<ElemType>) {
               if (end) {
                  return Return{std::nullopt};
               }
            }

            if constexpr (DRY_RUN) {
               if (error) {
                  return Return{std::nullopt};
               }
            }

            if constexpr (INDEX) {
               if constexpr (DRY_RUN) {
                  if (auto result = Find<NEXT>(json); result) {
                     json = *result;
                  } else {
                     error = true;
                     return Return{std::nullopt};
                  }
               } else {
                  json = Find<NEXT>(json);
               }
            }

            using SerializeType = OPTIONAL_VALUE_HELPER<ElemType>;

            if constexpr (IS_OPTIONAL<ElemType>) {
               if (auto result = Serializer<SerializeType, true>::Unserialize(json); result) {
                  auto [value, next] = *result;
                  json               = next;
                  return Return{std::move(value)};
               } else {
                  end = true;
                  return Return{std::nullopt};
               }
            } else if constexpr (DRY_RUN) {
               if (auto opt_result = Serializer<SerializeType, DRY_RUN>::Unserialize(json);
                   opt_result) {
                  auto [value, next] = *opt_result;
                  json               = next;
                  return Return{std::move(value)};
               } else {
                  error = true;
                  return Return{std::nullopt};
               }
            } else {
               auto [value, next] = Serializer<SerializeType, DRY_RUN>::Unserialize(json);
               json               = next;
               return Return{std::move(value)};
            }
         }()...};
      }(std::make_index_sequence<std::tuple_size_v<T>>());

      if constexpr (DRY_RUN && error) {
         return std::nullopt;
      }

      if constexpr (DRY_RUN) {
         if (auto result = Find<CLOSING>(json); result) {
            json = *result;
         } else {
            return std::nullopt;
         }
      } else {
         json = Find<CLOSING>(json);
      }

      return {result, json};
   }

   static constexpr std::string Serialize(T const& elem) noexcept(false) {
      return "[" +
             [&]<std::size_t... INDEX>(std::index_sequence<INDEX...>) constexpr {
                return (
                  [&]() constexpr {
                     using ElemType = std::tuple_element_t<INDEX, T>;

                     if constexpr (IS_OPTIONAL<ElemType>) {
                        if (!std::get<INDEX>(elem)) {
                           return "";
                        }

                        if constexpr (INDEX) {
                           return ","
                                  + Serializer<typename ElemType::value_type>::Serialize(
                                    *std::get<INDEX>(elem)
                                  );
                        } else {
                           return Serializer<typename ElemType::value_type>::Serialize(
                             *std::get<INDEX>(elem)
                           );
                        }
                     } else {
                        if constexpr (INDEX) {
                           return "," + Serializer<ElemType>::Serialize(std::get<INDEX>(elem));
                        } else {
                           return Serializer<ElemType>::Serialize(std::get<INDEX>(elem));
                        }
                     }
                  }()
                  + ... + ""_str
                );
             }(std::make_index_sequence<std::tuple_size_v<T>>())
             + "]";
   }
};
}  // namespace js
