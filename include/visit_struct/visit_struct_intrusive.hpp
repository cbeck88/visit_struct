//  (C) Copyright 2015 - 2016 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef VISIT_STRUCT_INTRUSIVE_HPP_INCLUDED
#define VISIT_STRUCT_INTRUSIVE_HPP_INCLUDED

/***
 * A collection of templates and macros supporting a second form of VISIT_STRUCT
 * mechanism.
 *
 * In this version, the visitable members are declared *within* the body of the
 * struct, at the same time that they are actually declared.
 *
 * This version uses templates for iteration rather than macros, so it's really
 * fairly different. It is more DRY and less likely to produce gross error
 * messages than the other, at the cost of being invasive to your structure
 * definition.
 *
 * This version adds some typedefs to your class, and it invisibly adds some
 * declarations of obscure member functions to your class. These declarations
 * do not have corresponding definitions and generate no object code, they are
 * merely a device for metaprogramming, exploiting overload resolution rules to
 * create "state". In normal code, you won't be able to detect any of this.
 */

#include <visit_struct/visit_struct.hpp>

namespace visit_struct {

namespace detail {

/***
 * Poor man's mpl vector
 */

template <class... Ts>
struct TypeList {
  static constexpr unsigned int size = sizeof...(Ts);
};

// Append metafunction
template <class List, class T>
struct Append;

template <class... Ts, class T>
struct Append<TypeList<Ts...>, T> {
  typedef TypeList<Ts..., T> type;
};

template<class L, class T>
using Append_t = typename Append<L, T>::type;

/***
 * The "rank" template is a trick which can be used for
 * certain metaprogramming techniques. It creates
 * an inheritance hierarchy of trivial classes.
 */

template <int N>
struct Rank : Rank<N - 1> {};

template <>
struct Rank<0> {};

static constexpr int maxVisitableRank = 200;

// A tag inserted into a structure to mark it as visitable

struct intrusive_tag{};

/***
 * Helper structures which perform pack expansion in order to visit a structure.
 */

template <typename M>
struct member_helper {
  template <typename V, typename S>
  VISIT_STRUCT_CONSTEXPR static void apply_visitor(V && visitor, S && structure_instance) {
    visitor(M::member_name, std::forward<S>(structure_instance).*(M::member_ptr));
  }
};

template <typename Mlist>
struct structure_helper;

template <typename... Ms>
struct structure_helper<TypeList<Ms...>> {
  template <typename V, typename S>
  VISIT_STRUCT_CONSTEXPR static void apply_visitor(V && visitor, S && structure_instance) {
    // Use parameter pack expansion to force evaluation of the member helper for each member in the list.
    // Inside parens, a comma operator is being used to discard the void value and produce an integer, while
    // not being an unevaluated context and having the order of evaluation be enforced by the compiler.
    // Extra zero at the end is to avoid UB for having a zero-size array.
    int dummy[] = { (member_helper<Ms>::apply_visitor(std::forward<V>(visitor), std::forward<S>(structure_instance)), 0)..., 0};
    // Suppress unused warnings, even in case of empty parameter pack
    static_cast<void>(dummy);
    static_cast<void>(visitor);
    static_cast<void>(structure_instance);
  }
};


} // end namespace detail


/***
 * Implement trait
 */

namespace traits {

template <typename T>
struct visitable <T,
                  typename std::enable_if<
                               std::is_same<typename T::Visit_Struct_Visitable_Structure_Tag__,
                                            ::visit_struct::detail::intrusive_tag
                                           >::value
                                         >::type
                 >
{
  template <typename V, typename S>
  VISIT_STRUCT_CONSTEXPR static void apply(V && v, S && s) {
    detail::structure_helper<typename T::Visit_Struct_Registered_Members_List__>::apply_visitor(std::forward<V>(v), std::forward<S>(s));
  }

  static constexpr bool value = true;
};

} // end namespace trait

} // end namespace visit_struct

// Macros to be used within a structure definition

#define VISIT_STRUCT_GET_REGISTERED_MEMBERS decltype(Visit_Struct_Get_Visitables__(::visit_struct::detail::Rank<visit_struct::detail::maxVisitableRank>{}))

#define VISIT_STRUCT_MAKE_MEMBER_NAME(NAME) Visit_Struct_Member_Record__##NAME

#define BEGIN_VISITABLES(NAME)                                                                                   \
typedef NAME VISIT_STRUCT_CURRENT_TYPE;                                                                          \
::visit_struct::detail::TypeList<> static inline Visit_Struct_Get_Visitables__(::visit_struct::detail::Rank<0>); \
static_assert(true, "")

#define VISITABLE(TYPE, NAME)                                                                                    \
TYPE NAME;                                                                                                       \
struct VISIT_STRUCT_MAKE_MEMBER_NAME(NAME) {                                                                     \
  static constexpr const char * const member_name = #NAME;                                                       \
  static constexpr const auto member_ptr = &VISIT_STRUCT_CURRENT_TYPE::NAME;                                     \
};                                                                                                               \
static inline ::visit_struct::detail::Append_t<VISIT_STRUCT_GET_REGISTERED_MEMBERS,                              \
                                               VISIT_STRUCT_MAKE_MEMBER_NAME(NAME)>                              \
  Visit_Struct_Get_Visitables__(::visit_struct::detail::Rank<VISIT_STRUCT_GET_REGISTERED_MEMBERS::size + 1>);    \
static_assert(true, "")

#define END_VISITABLES                                                                                           \
typedef VISIT_STRUCT_GET_REGISTERED_MEMBERS Visit_Struct_Registered_Members_List__;                              \
typedef ::visit_struct::detail::intrusive_tag Visit_Struct_Visitable_Structure_Tag__;                            \
static_assert(true, "")


#endif // VISIT_STRUCT_INTRUSIVE_HPP_INCLUDED
