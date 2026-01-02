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

#include <optional>
#include <string>
#include <type_traits>
#include <variant>

namespace js {

template <class T>
struct IsTupleT : std::false_type {};

template <class... T>
struct IsTupleT<std::tuple<T...>> : std::true_type {};

template <class T>
static constexpr bool IS_TUPLE = IsTupleT<T>::value;

template <class T>
struct IsOptional : std::false_type {};
template <class T>
struct IsOptional<std::optional<T>> : std::true_type {};
template <class T>
static constexpr bool IS_OPTIONAL = IsOptional<std::remove_cvref_t<T>>::value;

template <class T>
struct OptionalValueT {
   using type = T;
};

template <class T>
   requires(IS_OPTIONAL<T>)
struct OptionalValueT<T> {
   using type = typename T::value_type;
};

template <class T>
using OptionalValue = typename OptionalValueT<T>::type;

template <class T>
struct OptionalValueHelper {
   using type = T;
};

template <class T>
   requires(IS_OPTIONAL<T>)
struct OptionalValueHelper<T> {
   using type = typename T::value_type;
};

template <class T>
using OPTIONAL_VALUE_HELPER = typename OptionalValueHelper<T>::type;

template <class T>
struct IsVariant : std::false_type {};
template <class... T>
struct IsVariant<std::variant<T...>> : std::true_type {};

template <class T>
static constexpr bool IS_VARIANT = IsVariant<T>::value;

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
   IS_TUPLE<std::remove_cvref_t<decltype(T::PROTOTYPE)>>;
   is_true<([]<std::size_t... N>(std::index_sequence<N...>) consteval {
      return (is_member_pointer<T, decltype(std::get<N>(T::PROTOTYPE))> && ...);
   }(std::make_index_sequence<std::tuple_size_v<decltype(T::PROTOTYPE)>>()))>::value;
};

template <class T>
concept is_resizable = requires(T a) { a.emplace_back(std::declval<typename T::value_type>()); };
template <typename T>
static constexpr bool IS_STRING =
  std::is_constructible_v<std::remove_cvref_t<T>, std::string>
  && std::is_constructible_v<std::string_view, std::remove_cvref_t<T>>;

template <typename T>
concept is_map = requires(T t, T const CONST_T, typename T::key_type k) {
   typename T::key_type;
   typename T::mapped_type;
   IS_STRING<typename T::key_type>;
   { t[k] } -> std::same_as<typename T::mapped_type&>;
   { CONST_T.find(k) } -> std::same_as<typename T::const_iterator>;
};
}  // namespace js