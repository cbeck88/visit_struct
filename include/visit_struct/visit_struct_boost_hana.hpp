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

template <typename S>
struct visitable<S, typename std::enable_if<boost::hana::Struct<S>::value>::type>
{
  template <typename V>
  static void apply(V && v, const S & s) {
    boost::hana::for_each(s, [&v](auto pair) { v(boost::hana::to<char const *>(boost::hana::first(pair)), boost::hana::second(pair)); });
  }

  template <typename V>
  static void apply(V && v, S & s) {
    boost::hana::for_each(s, [&v](auto pair) { v(boost::hana::to<char const *>(boost::hana::first(pair)), boost::hana::second(pair)); });
  }

  template <typename V>
  static void apply(V && v, S && s) {
    boost::hana::for_each(s, [&v](auto pair) { v(boost::hana::to<char const *>(boost::hana::first(pair)), std::move(boost::hana::second(pair))); });
  }

  static constexpr bool value = true;
};

} // end namespace traits

} // end namespace visit_struct

#endif // VISIT_STRUCT_BOOST_HANA_HPP_INCLUDED
