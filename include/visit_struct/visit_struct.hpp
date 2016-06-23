//  (C) Copyright 2015 - 2016 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef VISIT_STRUCT_HPP_INCLUDED
#define VISIT_STRUCT_HPP_INCLUDED

/***
 * Provides a facility to declare a structure as "visitable" and apply a visitor
 * to it. The list of members is a compile-time data structure, and there is no
 * run-time overhead.
 */

#include <utility>
#include <type_traits>

# if (defined _MSC_VER) || (!defined __cplusplus) || (__cplusplus == 201103L)
#   define VISIT_STRUCT_CONSTEXPR
# else
#   define VISIT_STRUCT_CONSTEXPR constexpr
# endif

namespace visit_struct {

namespace traits {

// Primary template which is specialized to register a type
template <typename T, typename ENABLE = void>
struct visitable;

// Helper template which checks if a type is registered
template <typename T, typename ENABLE = void>
struct is_visitable : std::false_type {};

template <typename T>
struct is_visitable< T,
                     typename std::enable_if<traits::visitable<T>::value>::type>
 : std::true_type {};

} // end namespace traits

// Interface
template <typename S, typename V>
VISIT_STRUCT_CONSTEXPR auto apply_visitor(V && v, S && s) ->
  typename std::enable_if<
             traits::is_visitable<
               typename std::remove_cv<typename std::remove_reference<S>::type>::type
             >::value
           >::type
{
  using clean_S = typename std::remove_cv<typename std::remove_reference<S>::type>::type;
  traits::visitable<clean_S>::apply(std::forward<V>(v), std::forward<S>(s));
}

} // end namespace visit_struct

/***
 * This MAP macro was developed by William Swanson (c.f. 2012 https://github.com/swansontec/map-macro)
 *
 * It is a simple, self-contained macro which permits iteration over a list within the C preprocessor.
 *
 * Takes the name of a macro as first parameter, and then at most 365 additional arguments.
 * Evaluates to produce an invocation of the given macro on each of the arguments.
 * The arguments must be simple identifiers, they cannot contain commas or parentheses.
 */

#define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
#define VISIT_STRUCT_PP_MAP_EVAL1(...) VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(__VA_ARGS__)))
#define VISIT_STRUCT_PP_MAP_EVAL2(...) VISIT_STRUCT_PP_MAP_EVAL1(VISIT_STRUCT_PP_MAP_EVAL1(VISIT_STRUCT_PP_MAP_EVAL1(__VA_ARGS__)))
#define VISIT_STRUCT_PP_MAP_EVAL3(...) VISIT_STRUCT_PP_MAP_EVAL2(VISIT_STRUCT_PP_MAP_EVAL2(VISIT_STRUCT_PP_MAP_EVAL2(__VA_ARGS__)))
#define VISIT_STRUCT_PP_MAP_EVAL4(...) VISIT_STRUCT_PP_MAP_EVAL3(VISIT_STRUCT_PP_MAP_EVAL3(VISIT_STRUCT_PP_MAP_EVAL3(__VA_ARGS__)))
#define VISIT_STRUCT_PP_MAP_EVAL(...) VISIT_STRUCT_PP_MAP_EVAL4(VISIT_STRUCT_PP_MAP_EVAL4(VISIT_STRUCT_PP_MAP_EVAL4(__VA_ARGS__)))

#define VISIT_STRUCT_PP_MAP_END(...)
#define VISIT_STRUCT_PP_MAP_OUT

#define VISIT_STRUCT_PP_MAP_GET_END() 0, VISIT_STRUCT_PP_MAP_END
#define VISIT_STRUCT_PP_MAP_NEXT0(test, next, ...) next VISIT_STRUCT_PP_MAP_OUT
#define VISIT_STRUCT_PP_MAP_NEXT1(test, next) VISIT_STRUCT_PP_MAP_NEXT0(test, next, 0)
#define VISIT_STRUCT_PP_MAP_NEXT(test, next) VISIT_STRUCT_PP_MAP_NEXT1(VISIT_STRUCT_PP_MAP_GET_END test, next)

#define VISIT_STRUCT_PP_MAP0(f, x, peek, ...) f(x) VISIT_STRUCT_PP_MAP_NEXT(peek, VISIT_STRUCT_PP_MAP1)(f, peek, __VA_ARGS__)
#define VISIT_STRUCT_PP_MAP1(f, x, peek, ...) f(x) VISIT_STRUCT_PP_MAP_NEXT(peek, VISIT_STRUCT_PP_MAP0)(f, peek, __VA_ARGS__)
#define VISIT_STRUCT_PP_MAP(f, ...) VISIT_STRUCT_PP_MAP_EVAL(VISIT_STRUCT_PP_MAP1(f, __VA_ARGS__, (), 0))

/***
 * VISIT_STRUCT implementation details: 
 */

#define VISIT_STRUCT_MEMBER_HELPER(MEMBER_NAME)                                        \
  visitor(#MEMBER_NAME, struct_instance.MEMBER_NAME);

#define VISIT_STRUCT_MEMBER_HELPER_MOVE(MEMBER_NAME)                                   \
  visitor(#MEMBER_NAME, std::move(struct_instance.MEMBER_NAME));


// This macro specializes the trait, provides "apply" method which does the work.

#define VISITABLE_STRUCT(STRUCT_NAME, ...)                                                         \
namespace visit_struct {                                                                           \
namespace traits {                                                                                 \
                                                                                                   \
template <>                                                                                        \
struct visitable<STRUCT_NAME, void> {                                                              \
  template <typename V>                                                                            \
  VISIT_STRUCT_CONSTEXPR static void apply(V && visitor, STRUCT_NAME & struct_instance)            \
  {                                                                                                \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER, __VA_ARGS__)                                   \
  }                                                                                                \
                                                                                                   \
  template <typename V>                                                                            \
  VISIT_STRUCT_CONSTEXPR static void apply(V && visitor, const STRUCT_NAME & struct_instance)      \
  {                                                                                                \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER, __VA_ARGS__)                                   \
  }                                                                                                \
                                                                                                   \
  template <typename V>                                                                            \
  VISIT_STRUCT_CONSTEXPR static void apply(V && visitor, STRUCT_NAME && struct_instance)           \
  {                                                                                                \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER_MOVE, __VA_ARGS__)                              \
  }                                                                                                \
                                                                                                   \
  static constexpr bool value = true;                                                              \
};                                                                                                 \
                                                                                                   \
}                                                                                                  \
}                                                                                                  \
static_assert(true, "")

#endif // VISIT_STRUCT_HPP_INCLUDED
