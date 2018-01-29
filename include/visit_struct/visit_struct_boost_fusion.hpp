//  (C) Copyright 2015 - 2018 Christopher Beck

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
  // Get names for members of fusion structure S
  // Cage the unwieldy fusion syntax
  template <int idx>
  static VISIT_STRUCT_CONSTEXPR auto field_name() -> decltype(fusion::extension::struct_member_name<S, idx>::call()) {
    return fusion::extension::struct_member_name<S, idx>::call();
  }

  // Accessor type for fusion structure S
  template <int idx>
  struct accessor {
    VISIT_STRUCT_CONSTEXPR auto operator()(S & s) const ->
    decltype(fusion::at_c<idx>(s)) {
      return fusion::at_c<idx>(s);
    }

    VISIT_STRUCT_CONSTEXPR auto operator()(const S & s) const ->
    decltype(fusion::at_c<idx>(s)) {
      return fusion::at_c<idx>(s);
    }

    VISIT_STRUCT_CONSTEXPR auto operator()(S && s) const ->
    decltype(std::move(fusion::at_c<idx>(s))) {
      return std::move(fusion::at_c<idx>(s));
    }
  };

  // T is a qualified S
  // V should be a forwarding reference here, we should not be copying visitors
  template <typename V, typename T>
  struct fusion_visitor {
    V visitor;
    T struct_instance;

    explicit fusion_visitor(V v, T t) : visitor(std::forward<V>(v)), struct_instance(std::forward<T>(t)) {}

    template <typename Index>
    VISIT_STRUCT_CXX14_CONSTEXPR void operator()(Index) const {
      using accessor_t = accessor<Index::value>;
      std::forward<V>(visitor)(field_name<Index::value>(), accessor_t()(std::forward<T>(struct_instance)));
    }
  };

  template <typename V, typename T1, typename T2>
  struct fusion_visitor_pair {
    V visitor;
    T1 instance_1;
    T2 instance_2;

    explicit fusion_visitor_pair(V v, T1 t1, T2 t2)
      : visitor(std::forward<V>(v))
      , instance_1(std::forward<T1>(t1))
      , instance_2(std::forward<T2>(t2))
    {}

    template <typename Index>
    VISIT_STRUCT_CXX14_CONSTEXPR void operator()(Index) const {
      accessor<Index::value> a;
      std::forward<V>(visitor)(field_name<Index::value>(), a(std::forward<T1>(instance_1)), a(std::forward<T2>(instance_2)));
    }
  };

  template <typename V>
  struct fusion_visitor_types {
    V visitor;

    explicit fusion_visitor_types(V v) : visitor(std::forward<V>(v)) {}

    template <typename Index>
    VISIT_STRUCT_CXX14_CONSTEXPR void operator()(Index) const {
      using current_type = typename fusion::result_of::value_at<S, Index>::type;
      std::forward<V>(visitor)(field_name<Index::value>(), visit_struct::type_c<current_type>{});
    }
  };

  template <typename V>
  struct fusion_visitor_accessors {
    V visitor;

    explicit fusion_visitor_accessors(V v) : visitor(std::forward<V>(v)) {}

    template <typename Index>
    VISIT_STRUCT_CXX14_CONSTEXPR void operator()(Index) const {
      using accessor_t = accessor<Index::value>;
      std::forward<V>(visitor)(field_name<Index::value>(), accessor_t());
    }
  };

public:
  static VISIT_STRUCT_CONSTEXPR const size_t field_count = fusion::result_of::size<S>::value;

  // T should be a qualified S
  template <typename V, typename T>
  static void apply(V && v, T && t) {
    using Indices = mpl::range_c<unsigned, 0, fusion::result_of::size<S>::value >;
    using fv_t = fusion_visitor<decltype(std::forward<V>(v)), decltype(std::forward<T>(t))>;
    fv_t fv{std::forward<V>(v), std::forward<T>(t)};
    fusion::for_each(Indices(), fv);
  }

  template <typename V, typename T1, typename T2>
  static void apply(V && v, T1 && t1, T2 && t2) {
    using Indices = mpl::range_c<unsigned, 0, fusion::result_of::size<S>::value >;
    using fv_t = fusion_visitor_pair<decltype(std::forward<V>(v)), decltype(std::forward<T1>(t1)), decltype(std::forward<T2>(t2))>;
    fv_t fv{std::forward<V>(v), std::forward<T1>(t1), std::forward<T2>(t2)};
    fusion::for_each(Indices(), fv);
  }

  template <typename V>
  static void visit_types(V && v) {
    using Indices = mpl::range_c<unsigned, 0, fusion::result_of::size<S>::value >;
    using fv_t = fusion_visitor_types<decltype(std::forward<V>(v))>;
    fv_t fv{std::forward<V>(v)};
    fusion::for_each(Indices(), fv);
  }

  template <typename V>
  static void visit_accessors(V && v) {
    using Indices = mpl::range_c<unsigned, 0, fusion::result_of::size<S>::value >;
    using fv_t = fusion_visitor_accessors<decltype(std::forward<V>(v))>;
    fv_t fv{std::forward<V>(v)};
    fusion::for_each(Indices(), fv);
  }

  // T should be qualified S
  template <int idx, typename T>
  static VISIT_STRUCT_CONSTEXPR auto get_value(std::integral_constant<int, idx>, T && t)
    -> decltype(accessor<idx>()(std::forward<T>(t)))
  {
    return accessor<idx>()(std::forward<T>(t));
  }

  template <int idx>
  static VISIT_STRUCT_CONSTEXPR auto get_name(std::integral_constant<int, idx>)
    -> decltype(field_name<idx>())
  {
    return field_name<idx>();
  }

  template <int idx>
  static VISIT_STRUCT_CONSTEXPR auto get_accessor(std::integral_constant<int, idx>) ->
    accessor<idx>
  {
    return {};
  }

  template <int idx>
  static auto type_at(std::integral_constant<int, idx>) ->
    visit_struct::type_c<typename fusion::result_of::value_at_c<S, idx>::type>;

  static VISIT_STRUCT_CONSTEXPR const bool value = true;
};

} // end namespace traits

} // end namespace visit_struct

#endif // VISIT_STRUCT_BOOST_FUSION_HPP_INCLUDED
