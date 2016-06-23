//  (C) Copyright 2015 - 2016 Christopher Beck

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

template <typename S>
struct visitable<S,
                 typename std::enable_if<
                   std::is_same<typename boost::mpl::sequence_tag<S>::type,
                                boost::fusion::fusion_sequence_tag
                   >::value
                 >::type>
{

private:
  // T is a possible const / ref qualified version of S
  template <typename V, typename T>
  struct helper {
    V visitor;
    T struct_instance;

    explicit helper(V v, T t) : visitor(v), struct_instance(t) {}

    template <typename Index>
    void operator()(Index) const {
      visitor(boost::fusion::extension::struct_member_name<S, Index::value>::call(), boost::fusion::at<Index>(struct_instance));
    }
  };

  template <typename V, typename T>
  struct helper_rvalue_ref {
    V visitor;
    T struct_instance;

    explicit helper_rvalue_ref(V v, T t) : visitor(v), struct_instance(std::move(t)) {}

    template <typename Index>
    void operator()(Index) const {
      visitor(boost::fusion::extension::struct_member_name<S, Index::value>::call(), std::move(boost::fusion::at<Index>(struct_instance)));
    }
  };
    
public:
  template <typename V>
  static void apply(V && v, const S & s) {
    typedef boost::mpl::range_c<unsigned, 0, boost::fusion::result_of::size<S>::value > Indices; 
    helper<V, const S &> h{v, s};
    boost::fusion::for_each(Indices(), h);
  }

  template <typename V>
  static void apply(V && v, S & s) {
    typedef boost::mpl::range_c<unsigned, 0, boost::fusion::result_of::size<S>::value > Indices; 
    helper<V, S &> h{v, s};
    boost::fusion::for_each(Indices(), h);
  }

  template <typename V>
  static void apply(V && v, S && s) {
    typedef boost::mpl::range_c<unsigned, 0, boost::fusion::result_of::size<S>::value > Indices;
    helper_rvalue_ref<V, S &&> h{v, std::move(s)};
    boost::fusion::for_each(Indices(), h);
  }

  static constexpr bool value = true;
};

} // end namespace traits

} // end namespace visit_struct

#endif // VISIT_STRUCT_BOOST_FUSION_HPP_INCLUDED
