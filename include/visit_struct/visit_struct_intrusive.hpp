//  (C) Copyright 2015 - 2018 Christopher Beck

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
 *
 * This sounds a lot more evil than it really is -- it is morally equivalent to
 * `std::declval`, I would say, which is also specified to be a declaration with
 * no definition, which you simply aren't permitted to odr-use.
 */

#include <visit_struct/visit_struct.hpp>

namespace visit_struct {

namespace detail {

/***
 * Poor man's mpl vector
 */

template <class... Ts>
struct TypeList {
  static VISIT_STRUCT_CONSTEXPR const unsigned int size = sizeof...(Ts);
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

// Cdr metafunction (cdr is a lisp function which returns the tail of a list)
template <class List>
struct Cdr;

template <typename T, typename... Ts>
struct Cdr<TypeList<T, Ts...>> {
  typedef TypeList<Ts...> type;
};

template <class List>
using Cdr_t = typename Cdr<List>::type;

// Find metafunction (get the idx'th element)
template <class List, unsigned idx>
struct Find : Find<Cdr_t<List>, idx - 1> {};

template <typename T, typename... Ts>
struct Find<TypeList<T, Ts...>, 0> {
  typedef T type;
};

template <class List, unsigned idx>
using Find_t = typename Find<List, idx>::type;

// Alias used when capturing references to string literals

template <int N>
using char_array = const char [N];

/***
 * The "rank" template is a trick which can be used for
 * certain metaprogramming techniques. It creates
 * an inheritance hierarchy of trivial classes.
 */

template <int N>
struct Rank : Rank<N - 1> {};

template <>
struct Rank<0> {};

static VISIT_STRUCT_CONSTEXPR const int max_visitable_members_intrusive = 100;

/***
 * To create a "compile-time" TypeList whose members are accumulated one-by-one,
 * the basic idea is to define a function, which takes a `Rank` object, and
 * whose return type is the type representing the current value of the list.
 *
 * That function is not a template function -- it is defined as taking a
 * particular rank object. Initially, it is defined only for `Rank<0>`.
 *
 * To add an element to the list, we define an overload of the function, which
 * takes the next higher `Rank` as it's argument. It's return value is,
 * the new value of the list, formed by using `Append_t` with the old value.
 *
 * To obtain the current value of the list, we use decltype with the name of the
 * function, and `Rank<100>`, or some suitably large integer. The C++ standard
 * specifies that overload resolution is in this case unambiguous and must
 * select the overload for the "most-derived" type which matches.
 *
 * The upshot is that `decltype(my_function(Rank<100>{}))` is a single well-formed
 * expression, which, because of C++ overload resolution rules, can be a
 * "mutable" value from the point of view of metaprogramming.
 *
 *
 * Attribution:
 * I first learned this trick from a stackoverflow post by Roman Perepelitsa:
 * http://stackoverflow.com/questions/4790721/c-type-registration-at-compile-time-trick
 *
 * He attributes it to a talk from Matt Calabrese at BoostCon 2011.
 *
 *
 * The expression is inherently dangerous if you are using it inside the body
 * of a struct -- obviously, it has different values at different points of the
 * structure definition. The "END_VISITABLES" macro is important in that this
 * finalizes the list, typedeffing `decltype(my_function(Rank<100>{}))` to some
 * fixed name in your struct at a specific point in the definition. That
 * typedef can only ultimately have one meaning, no matter where else the name
 * may be used (even implicitly) in your structure definition. That typedef is
 * what the trait defined in this header ultimately hooks into to find the
 * visitable members.
 */

// A tag inserted into a structure to mark it as visitable

struct intrusive_tag{};

/***
 * Helper structures which perform pack expansion in order to visit a structure.
 */

// In MSVC 2015, sometimes a pointer to member cannot be constexpr, for instance
// I had trouble with code like this:
//
// struct S {
//   int a;
//   static constexpr auto a_ptr = &S::a;
// };
//
// This works fine in gcc and clang.
// MSVC is okay with it if instead it is a template parameter it seems, so we
// use `member_ptr_helper` below as a workaround, a bit like so:
//
// struct S {
//   int a;
//   using a_helper = member_ptr_helper<S, int, &S::a>;
// };

template <typename S, typename T, T S::*member_ptr>
struct member_ptr_helper {
  static VISIT_STRUCT_CONSTEXPR T S::* get_ptr() { return member_ptr; }
  using value_type = T;

  using accessor_t = visit_struct::accessor<T S::*, member_ptr>;
};

// M should be derived from a member_ptr_helper
template <typename M>
struct member_helper {
  template <typename V, typename S>
  VISIT_STRUCT_CXX14_CONSTEXPR static void apply_visitor(V && visitor, S && structure_instance) {
    std::forward<V>(visitor)(M::member_name(), std::forward<S>(structure_instance).*M::get_ptr());
  }

  template <typename V, typename S1, typename S2>
  VISIT_STRUCT_CXX14_CONSTEXPR static void apply_visitor(V && visitor, S1 && s1, S2 && s2) {
    std::forward<V>(visitor)(M::member_name(),
                             std::forward<S1>(s1).*M::get_ptr(),
                             std::forward<S2>(s2).*M::get_ptr());
  }

  template <typename V>
  VISIT_STRUCT_CXX14_CONSTEXPR static void visit_pointers(V && visitor) {
    std::forward<V>(visitor)(M::member_name(), M::get_ptr());
  }

  template <typename V>
  VISIT_STRUCT_CXX14_CONSTEXPR static void visit_accessors(V && visitor) {
    std::forward<V>(visitor)(M::member_name(), typename M::accessor_t());
  }

  template <typename V>
  VISIT_STRUCT_CXX14_CONSTEXPR static void visit_types(V && visitor) {
    std::forward<V>(visitor)(M::member_name(), visit_struct::type_c<typename M::value_type>{});
  }

};

template <typename Mlist>
struct structure_helper;

template <typename... Ms>
struct structure_helper<TypeList<Ms...>> {
  template <typename V, typename S>
  VISIT_STRUCT_CXX14_CONSTEXPR static void apply_visitor(V && visitor, S && structure_instance) {
    // Use parameter pack expansion to force evaluation of the member helper for each member in the list.
    // Inside parens, a comma operator is being used to discard the void value and produce an integer, while
    // not being an unevaluated context. The order of evaluation here is enforced by the compiler.
    // Extra zero at the end is to avoid UB for having a zero-size array.
    int dummy[] = {(member_helper<Ms>::apply_visitor(std::forward<V>(visitor), std::forward<S>(structure_instance)), 0)..., 0};
    // Suppress unused warnings, even in case of empty parameter pack
    static_cast<void>(dummy);
    static_cast<void>(visitor);
    static_cast<void>(structure_instance);
  }

  template <typename V, typename S1, typename S2>
  VISIT_STRUCT_CXX14_CONSTEXPR static void apply_visitor(V && visitor, S1 && s1, S2 && s2) {
    int dummy[] = {(member_helper<Ms>::apply_visitor(std::forward<V>(visitor), std::forward<S1>(s1), std::forward<S2>(s2)), 0)..., 0};
    static_cast<void>(dummy);
    static_cast<void>(visitor);
    static_cast<void>(s1);
    static_cast<void>(s2);
  }

  template <typename V>
  VISIT_STRUCT_CXX14_CONSTEXPR static void visit_pointers(V && visitor) {
    int dummy[] = {(member_helper<Ms>::visit_pointers(std::forward<V>(visitor)), 0)..., 0};
    static_cast<void>(dummy);
    static_cast<void>(visitor);
  }

  template <typename V>
  VISIT_STRUCT_CXX14_CONSTEXPR static void visit_accessors(V && visitor) {
    int dummy[] = {(member_helper<Ms>::visit_accessors(std::forward<V>(visitor)), 0)..., 0};
    static_cast<void>(dummy);
    static_cast<void>(visitor);
  }

  template <typename V>
  VISIT_STRUCT_CXX14_CONSTEXPR static void visit_types(V && visitor) {
    int dummy[] = {(member_helper<Ms>::visit_types(std::forward<V>(visitor)), 0)..., 0};
    static_cast<void>(dummy);
    static_cast<void>(visitor);
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
  static VISIT_STRUCT_CONSTEXPR const std::size_t field_count = T::Visit_Struct_Registered_Members_List__::size;

  // Apply to an instance
  // S should be the same type as T modulo const and reference
  template <typename V, typename S>
  static VISIT_STRUCT_CXX14_CONSTEXPR void apply(V && v, S && s) {
    detail::structure_helper<typename T::Visit_Struct_Registered_Members_List__>::apply_visitor(std::forward<V>(v), std::forward<S>(s));
  }

  // Apply with two instances
    template <typename V, typename S1, typename S2>
  static VISIT_STRUCT_CXX14_CONSTEXPR void apply(V && v, S1 && s1, S2 && s2) {
    detail::structure_helper<typename T::Visit_Struct_Registered_Members_List__>::apply_visitor(std::forward<V>(v), std::forward<S1>(s1), std::forward<S2>(s2));
  }

  // Apply with no instance
  template <typename V>
  static VISIT_STRUCT_CXX14_CONSTEXPR void visit_pointers(V && v) {
    detail::structure_helper<typename T::Visit_Struct_Registered_Members_List__>::visit_pointers(std::forward<V>(v));
  }

  template <typename V>
  static VISIT_STRUCT_CXX14_CONSTEXPR void visit_types(V && v) {
    detail::structure_helper<typename T::Visit_Struct_Registered_Members_List__>::visit_types(std::forward<V>(v));
  }

  template <typename V>
  static VISIT_STRUCT_CXX14_CONSTEXPR void visit_accessors(V && v) {
    detail::structure_helper<typename T::Visit_Struct_Registered_Members_List__>::visit_accessors(std::forward<V>(v));
  }

  // Get pointer
  template <int idx>
  static VISIT_STRUCT_CONSTEXPR auto get_pointer(std::integral_constant<int, idx>)
    -> decltype(detail::Find_t<typename T::Visit_Struct_Registered_Members_List__, idx>::get_ptr())
  {
    return detail::Find_t<typename T::Visit_Struct_Registered_Members_List__, idx>::get_ptr();
  }

  // Get accessor
  template <int idx>
  static VISIT_STRUCT_CONSTEXPR auto get_accessor(std::integral_constant<int, idx>)
    -> typename detail::Find_t<typename T::Visit_Struct_Registered_Members_List__, idx>::accessor_t
  {
    return {};
  }

  // Get value
  template <int idx, typename S>
  static VISIT_STRUCT_CONSTEXPR auto get_value(std::integral_constant<int, idx> tag, S && s)
    -> decltype(std::forward<S>(s).*get_pointer(tag))
  {
    return std::forward<S>(s).*get_pointer(tag);
  }

  // Get name
  template <int idx>
  static VISIT_STRUCT_CONSTEXPR auto get_name(std::integral_constant<int, idx>)
    -> decltype(detail::Find_t<typename T::Visit_Struct_Registered_Members_List__, idx>::member_name())
  {
    return detail::Find_t<typename T::Visit_Struct_Registered_Members_List__, idx>::member_name();
  }

  // Get type
  template <int idx>
  static auto type_at(std::integral_constant<int, idx>)
    -> visit_struct::type_c<typename detail::Find_t<typename T::Visit_Struct_Registered_Members_List__, idx>::value_type>;

  // Get name of structure
  static VISIT_STRUCT_CONSTEXPR decltype(T::Visit_Struct_Get_Name__()) get_name() {
    return T::Visit_Struct_Get_Name__();
  }

  static VISIT_STRUCT_CONSTEXPR const bool value = true;
};

} // end namespace trait

} // end namespace visit_struct

// Macros to be used within a structure definition

#define VISIT_STRUCT_GET_REGISTERED_MEMBERS decltype(Visit_Struct_Get_Visitables__(::visit_struct::detail::Rank<visit_struct::detail::max_visitable_members_intrusive>{}))

#define VISIT_STRUCT_MAKE_MEMBER_NAME(NAME) Visit_Struct_Member_Record__##NAME

#define BEGIN_VISITABLES(NAME)                                                                                   \
typedef NAME VISIT_STRUCT_CURRENT_TYPE;                                                                          \
static VISIT_STRUCT_CONSTEXPR decltype(#NAME) Visit_Struct_Get_Name__() {                                        \
  return #NAME;                                                                                                  \
}                                                                                                                \
::visit_struct::detail::TypeList<> static inline Visit_Struct_Get_Visitables__(::visit_struct::detail::Rank<0>); \
static_assert(true, "")

#define VISITABLE(TYPE, NAME)                                                                                    \
TYPE NAME;                                                                                                       \
struct VISIT_STRUCT_MAKE_MEMBER_NAME(NAME) :                                                                     \
  visit_struct::detail::member_ptr_helper<VISIT_STRUCT_CURRENT_TYPE,                                             \
                                          TYPE,                                                                  \
                                          &VISIT_STRUCT_CURRENT_TYPE::NAME>                                      \
{                                                                                                                \
  static VISIT_STRUCT_CONSTEXPR const ::visit_struct::detail::char_array<sizeof(#NAME)> & member_name() {        \
    return #NAME;                                                                                                \
  }                                                                                                              \
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
