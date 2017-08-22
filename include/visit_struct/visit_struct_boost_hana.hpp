//  (C) Copyright 2015 - 2017 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef VISIT_STRUCT_BOOST_HANA_HPP_INCLUDED
#define VISIT_STRUCT_BOOST_HANA_HPP_INCLUDED

#include <visit_struct/visit_struct.hpp>

#include <boost/hana.hpp>
#include <utility>

namespace visit_struct {

namespace traits {

namespace hana = boost::hana;

template <typename S>
struct visitable<S, typename std::enable_if<hana::Struct<S>::value>::type>
{
  static constexpr size_t field_count = decltype(hana::length(hana::accessors<S>()))::value;

  // Note: U should be qualified S (const and references) S, interface should ensure that
  template <typename V, typename U>
  static constexpr void apply(V && v, U && u) {
    hana::for_each(hana::accessors<S>(), [&v, &u](auto pair) {
      std::forward<V>(v)(hana::to<char const *>(hana::first(pair)), hana::second(pair)(std::forward<U>(u)));
    });
  }

  template <typename V, typename U1, typename U2>
  static constexpr void apply(V && v, U1 && u1, U2 && u2) {
    hana::for_each(hana::accessors<S>(), [&v, &u1, &u2](auto pair) {
      std::forward<V>(v)(hana::to<char const *>(hana::first(pair)),
                         hana::second(pair)(std::forward<U1>(u1)),
                         hana::second(pair)(std::forward<U2>(u2)));
    });
  }

  template <typename V>
  static constexpr void visit_types(V && v) {
    hana::for_each(hana::accessors<S>(), [&v](auto pair) {
      // Finding the declared type of the member with hana is tricky because hana doesn't expose that directly.
      // It only gives us the accessor function, which always returns references.
      // We need to be able to distinguish between, the declared member is "int" and accessor is returning "int &"
      // and, the declared member is "int &" and the accessor is returning "int &".
      // We do this by passing const and non-const versions of the struct, and check if the accessed type is changing.
      // If the type is changing, it means the member is a value type, so we should clean it.
      // If the type is not changing, it means the member has a reference type, and we should not clean it

      using test_one = decltype(hana::second(pair)(*static_cast<S *>(nullptr)));
      using test_two = decltype(hana::second(pair)(*static_cast<const S *>(nullptr)));

      using member_type = typename std::conditional<std::is_same<test_one, test_two>::value,
                                                    test_one,
                                                    visit_struct::traits::clean_t<test_one>>::type;

      std::forward<V>(v)(hana::to<char const *>(hana::first(pair)),
                         visit_struct::type_c<member_type>{});
    });
  }

  template <typename V>
  static constexpr void visit_accessors(V && v) {
    hana::for_each(hana::accessors<S>(), [&v](auto pair) {
      std::forward<V>(v)(hana::to<char const *>(hana::first(pair)),
                         hana::second(pair));
    });
  }

  template <int idx, typename T>
  static constexpr auto get_value(std::integral_constant<int, idx>, T && t) ->
    decltype(hana::second(hana::at(hana::accessors<S>(), hana::size_c<idx>)) (std::forward<T>(t))) {
    return hana::second(hana::at(hana::accessors<S>(), hana::size_c<idx>)) (std::forward<T>(t));
  }

  template <int idx>
  static constexpr auto get_name(std::integral_constant<int, idx>) -> const char * {
    return hana::to<const char *>(hana::first(hana::at(hana::accessors<S>(), hana::size_c<idx>)));
  }

  template <int idx>
  static constexpr auto get_accessor(std::integral_constant<int, idx>) ->
    decltype(hana::second(hana::at(hana::accessors<S>(), hana::size_c<idx>))) {
    return hana::second(hana::at(hana::accessors<S>(), hana::size_c<idx>));
  }

  static constexpr bool value = true;
};

} // end namespace traits

} // end namespace visit_struct

#endif // VISIT_STRUCT_BOOST_HANA_HPP_INCLUDED
