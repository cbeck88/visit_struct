//  (C) Copyright 2015 - 2016 Christopher Beck

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
  template <typename V>
  static void apply(V && v, const S & s) {
    hana::for_each(hana::keys(s), [&v, &s](auto key) {
      std::forward<V>(v)(hana::to<char const *>(key), hana::at_key(s, key));
    });
  }

  template <typename V>
  static void apply(V && v, S & s) {
    hana::for_each(hana::keys(s), [&v, &s](auto key) {
      std::forward<V>(v)(hana::to<char const *>(key), hana::at_key(s, key));
    });
  }

  template <typename V>
  static void apply(V && v, S && s) {
    hana::for_each(hana::keys(s), [&v, &s](auto key) {
      std::forward<V>(v)(hana::to<char const *>(key), std::move(hana::at_key(s, key)));
    });
  }

  static constexpr bool value = true;
};

} // end namespace traits

} // end namespace visit_struct

#endif // VISIT_STRUCT_BOOST_HANA_HPP_INCLUDED
