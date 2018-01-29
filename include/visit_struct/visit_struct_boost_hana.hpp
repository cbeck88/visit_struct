//  (C) Copyright 2015 - 2018 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef VISIT_STRUCT_BOOST_HANA_HPP_INCLUDED
#define VISIT_STRUCT_BOOST_HANA_HPP_INCLUDED

#include <visit_struct/visit_struct.hpp>

#include <boost/hana.hpp>
#include <utility>

namespace visit_struct {

namespace traits {

// Accessor value-type
// An accessor is a function object which encapsulates a pointer to member
// It is generally an overloaded function object:
// If the pointer type is T S::*, then the function object has the following overloads:
// S & -> T &
// const S & -> const T &
// S && -> T &&
//
// When using boost::hana, a struct is defined *only* in terms of a sequence of names and
// accessors. There is no built-in functionality to obtain the actual member-type T.
// And the accessor is not required to provide a typedef exposing it.
//
// We may attempt to infer it, however, by a decltype expression where we evaluate
// the accessor. Then we can remove const and reference.
//
// However, what if the struct actually contains const members, or reference members?
// If the member type is actually T & and not T, then the accessor yields:
//
// S & -> T &
// const S & -> T &
// S && -> T &
//
// In other words, if the member type is a reference type, then the const and ref qualifiers of input
// don't matter.
//
// If the member type is actually const T, then the accessor yields:
//
// S & -> const T &
// const S & -> const T &
// S && -> const T &&
//
// Therefore a viable way to infer the value type from the accessor is
//   1. Check the output types of accessor(S&) and accessor(S&&)
//   2. If they are the same, the value type is that type
//   3. If they are different, then remove reference from one of them to obtain the value type. (It shouldn't matter which one.)
//
// It's not clear what we should do if, after removing reference from the two test types, we don't get the same type.
// In current versions we don't check if that's actually the case. It would be reasonable to static_assert that it doesn't happen I think.
//
// Also it's worth noting that hana doesn't apparently support structs with reference members, and test code fails compilation
// with "error: cannot create pointer to reference member" in gcc-6. So this is probably overkill.

template <typename A, typename S>
struct accessor_value_type {
  using test1 = decltype(std::declval<A>()(std::declval<S&>()));
  using test2 = decltype(std::declval<A>()(std::declval<S&&>()));
  
  using type = typename std::conditional<std::is_same<test1, test2>::value,
                        test1,
                        typename std::remove_reference<test1>::type>::type;
};

template <typename A, typename S>
using accessor_value_t = typename accessor_value_type<A, S>::type;

// Hana bindings start here

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
      using member_type = accessor_value_t<decltype(hana::second(pair)), S>;
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

  template <int idx>
  static auto type_at(std::integral_constant<int, idx>) ->
    visit_struct::type_c<accessor_value_t<decltype(hana::second(hana::at(hana::accessors<S>(), hana::size_c<idx>))), S>>;

  static constexpr bool value = true;
};

} // end namespace traits

} // end namespace visit_struct

#endif // VISIT_STRUCT_BOOST_HANA_HPP_INCLUDED
