#include <visit_struct/visit_struct.hpp>

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
 * This is an illustration of how visit_struct can be used to do
 * some nontrivial compile-time metaprogramming.
 *
 * (Old versions required C++14, for constexpr visitation, but using
 *  visit_struct::type_at, we can now do it with only C++11.)
 *
 * Potentially, you could use it to catch bugs at compile-time which occur
 * if e.g. a member is added to a struct but the programmer forgets to add it
 * to the VISITABLE_STRUCT macro also.
 *
 * Tested against gcc 4.8, 4.9, 5.4, 6.2 and clang 3.5, 3.8
 */

namespace ext {

// C++11 replacement for std::index_sequence
template <std::size_t ...>
struct seq {};

// Concatenate sequences
template <class, class>
struct cat_s;

template <std::size_t ... as, std::size_t ... bs>
struct cat_s<seq<as...>, seq<bs...>> {
  using type = seq<as..., bs...>;
};

template <class s1, class s2>
using cat = typename cat_s<s1, s2>::type;

// Count

template <std::size_t s>
struct count_s {
  using type = cat<typename count_s<s-1>::type, seq<s-1>>;
};

template <>
struct count_s<0> {
  using type = seq<>;
};

template <std::size_t s>
using count = typename count_s<s>::type;


// It turns out it is implementation-defined whether `std::tuple` lists its members
// in order specified or in reverse order, and some standard libraries do it differently.
// So we have to make it up a bit here for our intended application, and we don't need
// full tuple interface anyways, we only need the size to be correct.
// This is based on libc++ implementation.

// Note: Extra std::size_t parameter is here to avoid "duplicate base type is invalid" error
template <typename T, std::size_t>
struct mock_tuple_leaf { T t; };

template <typename I, typename ... Ts>
struct mock_tuple;

template <typename ... Ts, std::size_t ... Is>
struct mock_tuple<seq<Is...>, Ts...> : mock_tuple_leaf<Ts, Is> ... {};

template <typename ... Ts>
using mock_tuple_t = mock_tuple<count<sizeof...(Ts)>, Ts...>;

// Build mock from a visitable structure
// Does the work

template <typename T>
struct mock_maker {
  template <typename is>
  struct helper;

  template <std::size_t ... Is>
  struct helper<seq< Is...>> {
    using type = mock_tuple_t<visit_struct::type_at<Is, T> ...>;
  };
  
  using type = typename helper<count<visit_struct::field_count<T>()>>::type;
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
