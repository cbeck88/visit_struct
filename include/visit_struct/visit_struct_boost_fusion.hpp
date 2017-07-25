//  (C) Copyright 2015 - 2017 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef VISIT_STRUCT_BOOST_FUSION_HPP_INCLUDED
#define VISIT_STRUCT_BOOST_FUSION_HPP_INCLUDED

#include <visit_struct/visit_struct.hpp>

#include <boost/mpl/range_c.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/zip.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/adapted.hpp>
#include <boost/fusion/include/mpl.hpp>

#include <utility>

namespace visit_struct {

namespace traits {

namespace fusion = boost::fusion;
namespace mpl = boost::mpl;

template <typename S>
struct visitable<S,
                 typename std::enable_if<
                   std::is_same<typename mpl::sequence_tag<S>::type,
                                fusion::fusion_sequence_tag
                   >::value
                 >::type>
{

private:
  // T is a possible const / ref qualified version of S
  template <typename V, typename T>
  struct helper {
    V visitor;
    T struct_instance;

    explicit helper(V v, T t) : visitor(std::forward<V>(v)), struct_instance(t) {}

    template <typename Index>
    void operator()(Index) const {
      std::forward<V>(visitor)(fusion::extension::struct_member_name<S, Index::value>::call(), fusion::at<Index>(struct_instance));
    }
  };

  template <typename V, typename T>
  struct helper_rvalue_ref {
    V visitor;
    T struct_instance;

    explicit helper_rvalue_ref(V v, T t) : visitor(std::forward<V>(v)), struct_instance(std::move(t)) {}

    template <typename Index>
    void operator()(Index) const {
      std::forward<V>(visitor)(fusion::extension::struct_member_name<S, Index::value>::call(), std::move(fusion::at<Index>(struct_instance)));
    }
  };
    
public:
  static VISIT_STRUCT_CONSTEXPR const size_t field_count = fusion::result_of::size<S>::value;

  template <typename V>
  static void apply(V && v, const S & s) {
    using Indices = mpl::range_c<unsigned, 0, fusion::result_of::size<S>::value >;
    using helper_t = helper<decltype(std::forward<V>(v)), const S &>;
    helper_t h{std::forward<V>(v), s};
    fusion::for_each(Indices(), h);
  }

  template <typename V>
  static void apply(V && v, S & s) {
    using Indices = mpl::range_c<unsigned, 0, fusion::result_of::size<S>::value >;
    using helper_t = helper<decltype(std::forward<V>(v)), S &>;
    helper_t h{std::forward<V>(v), s};
    fusion::for_each(Indices(), h);
  }

  template <typename V>
  static void apply(V && v, S && s) {
    using Indices = mpl::range_c<unsigned, 0, fusion::result_of::size<S>::value >;
    using helper_t = helper_rvalue_ref<decltype(std::forward<V>(v)), S &&>;
    helper_t h{std::forward<V>(v), std::move(s)};
    fusion::for_each(Indices(), h);
  }

  template <int idx>
  static VISIT_STRUCT_CXX14_CONSTEXPR auto get_value(std::integral_constant<int, idx>, S & s)
    -> decltype(fusion::at_c<idx>(s))
  {
    return fusion::at_c<idx>(s);
  }

  template <int idx>
  static VISIT_STRUCT_CXX14_CONSTEXPR auto get_value(std::integral_constant<int, idx>, const S & s)
    -> decltype(fusion::at_c<idx>(s))
  {
    return fusion::at_c<idx>(s);
  }

  template <int idx>
  static VISIT_STRUCT_CXX14_CONSTEXPR auto get_value(std::integral_constant<int, idx>, S && s)
    -> decltype(std::move(fusion::at_c<idx>(s)))
  {
    return std::move(fusion::at_c<idx>(s));
  }

  template <int idx>
  static VISIT_STRUCT_CXX14_CONSTEXPR const char * get_name(std::integral_constant<int, idx>) {
    return fusion::extension::struct_member_name<S, idx>::call();
  }

  static VISIT_STRUCT_CONSTEXPR const bool value = true;
};

} // end namespace traits

} // end namespace visit_struct

#endif // VISIT_STRUCT_BOOST_FUSION_HPP_INCLUDED
