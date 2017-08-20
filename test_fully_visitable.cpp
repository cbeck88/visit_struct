#include <visit_struct/visit_struct.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <type_traits>

/***
 * Code in `namespace ext` implements a type trait, "is_fully_visitable".
 *
 * This is a compile-time test which attempts to verify that a visitable structure
 * has all of its fields registered.
 *
 * It does this by checking the sizeof the structure vs what the size should be
 * if the visitable fields are all of the fields.
 *
 * This is an illustration of how visit_struct can be used with C++14 to do
 * some nontrivial compile-time metaprogramming.
 *
 * Potentially, you could use it to catch bugs at compile-time which occur
 * if e.g. a member is added to a struct but the programmer forgets to add it
 * to the VISITABLE_STRUCT macro also.
 *
 * Tested against gcc 6.2.0 and clang 3.8.0
 */

namespace ext {

// Get size / alignment at a particular index of visitable structure

struct size_visitor {
  std::size_t idx = 0;
  std::size_t count = 0;
  std::size_t result = 0;

  template <typename T, typename S>
  constexpr void operator()(const char *, T S::*) {
    if (idx == count++) {
      result = sizeof(T);
    }
  }
};

struct align_visitor {
  std::size_t idx = 0;
  std::size_t count = 0;
  std::size_t result = 0;

  template <typename T, typename S>
  constexpr void operator()(const char *, T S::*) {
    if (idx == count++) {
      result = alignof(T);
    }
  }
};

template <typename T, std::size_t idx>
constexpr std::size_t size_at() {
  size_visitor vis;
  vis.idx = idx;
  visit_struct::visit_pointers<T>(vis);
  return vis.result;
}

template <typename T, std::size_t idx>
constexpr std::size_t align_at() {
  align_visitor vis;
  vis.idx = idx;
  visit_struct::visit_pointers<T>(vis);
  return vis.result;
}



// It turns out it is implementation-defined whether `std::tuple` lists its members
// in order specified or in reverse order, and some standard libraries do it differently.
// So we have to make it up a bit here for our intended application, and we don't need
// full tuple interface anyways, we only need the size to be correct.
// This is based on libc++ implementation.

template <typename T, std::size_t>
struct mock_tuple_leaf { T t; };

template <typename I, typename ... Ts>
struct mock_tuple;

template <typename ... Ts, std::size_t ... Is>
struct mock_tuple<std::integer_sequence<std::size_t, Is...>, Ts...> : mock_tuple_leaf<Ts, Is> ... {};

template <typename ... Ts>
using mock_tuple_t = mock_tuple<std::make_index_sequence<sizeof...(Ts)>, Ts...>;

// Build mock from a visitable structure
// Does the work

template <std::size_t s, std::size_t a>
using mock_field_t = typename std::aligned_storage<s, a>::type;

template <typename T>
struct mock_maker {
  template <typename is>
  struct helper;

  template <std::size_t ... Is>
  struct helper<std::integer_sequence<std::size_t, Is...>> {
    using type = mock_tuple_t<mock_field_t<size_at<T, Is>(), align_at<T, Is>()> ...>;
  };
  
  using type = typename helper<std::make_index_sequence<visit_struct::field_count<T>()>>::type;
  static constexpr std::size_t size = sizeof(type);
};

// Final result

template <typename T>
constexpr bool is_fully_visitable() {
  return sizeof(T) == mock_maker<T>::size;
}

} // end namespace ext




// Tests

struct foo {
  int a;
  int b;
  int c;
};

VISITABLE_STRUCT(foo, a, b, c);

static_assert(sizeof(foo) == 3 * sizeof(int), "");
static_assert(visit_struct::field_count<foo>() == 3, "");
static_assert(ext::size_at<foo, 0>() == sizeof(int), "");
static_assert(ext::size_at<foo, 1>() == sizeof(int), "");
static_assert(ext::size_at<foo, 2>() == sizeof(int), "");
static_assert(ext::align_at<foo, 0>() == alignof(int), "");
static_assert(ext::align_at<foo, 1>() == alignof(int), "");
static_assert(ext::align_at<foo, 2>() == alignof(int), "");
static_assert(ext::mock_maker<foo>::size == 3 * sizeof(int), "");

static_assert(ext::is_fully_visitable<foo>(), "");

struct bar {
  int a;
  int b;
  int c;
};

VISITABLE_STRUCT(bar, a, b);

static_assert(!ext::is_fully_visitable<bar>(), "");


struct baz {
  int a;
  char b[7];
  short c;
  char d[157];
};

VISITABLE_STRUCT(baz, a, b, c, d);

static_assert(ext::is_fully_visitable<baz>(), "");

int main () {}
