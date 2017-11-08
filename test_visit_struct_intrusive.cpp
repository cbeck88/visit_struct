#include <visit_struct/visit_struct_intrusive.hpp>

#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

namespace test {

  struct foo {
    BEGIN_VISITABLES(foo);
    VISITABLE(bool, b);
    VISITABLE(int, i);
    VISITABLE(float, f);
    END_VISITABLES;
  };

} // end namespace test

static_assert(visit_struct::field_count<test::foo>() == 3, "");

struct test_visitor_one {
  std::vector<std::string> names;
  std::vector<double> values;

  template <typename T>
  void operator()(const char * name, T v) {
    names.emplace_back(name);
    values.emplace_back(v);
  }
};

using spair = std::pair<std::string, std::string>;

struct test_visitor_ptr {
  std::vector<spair> result;

  template <typename C>
  void operator()(const char* name, int C::*) {
    result.emplace_back(spair{std::string{name}, "int"});
  }

  template <typename C>
  void operator()(const char* name, float C::*) {
    result.emplace_back(spair{std::string{name}, "float"});
  }

  template <typename C>
  void operator()(const char* name, bool C::*) {
    result.emplace_back(spair{std::string{name}, "bool"});
  }
};

struct test_visitor_type {
  std::vector<spair> result;

  void operator()(const char* name, visit_struct::type_c<int>) {
    result.emplace_back(spair{std::string{name}, "int"});
  }

  void operator()(const char* name, visit_struct::type_c<float>) {
    result.emplace_back(spair{std::string{name}, "float"});
  }

  void operator()(const char* name, visit_struct::type_c<bool>) {
    result.emplace_back(spair{std::string{name}, "bool"});
  }
};


template <typename S>
struct test_visitor_acc {
  test_visitor_type internal;

  template <typename A>
  void operator()(const char* name, A && a) {
    using result_t = visit_struct::traits::clean_t<decltype(a(std::declval<S>()))>;
    internal(name, visit_struct::type_c<result_t>{});
  }
};



struct test_visitor_three {
  int result = 0;

  template <typename T>
  void operator()(const char *, T &&) {}

  void operator()(const char *, int &) {
    result = 1;
  }

  void operator()(const char *, const int &) {
    result = 2;
  }

  void operator()(const char *, int &&) {
    result = 3;
  }

  // Make it non-copyable and non-moveable, apply visitor should still work.
  test_visitor_three() = default;
  test_visitor_three(const test_visitor_three &) = delete;
  test_visitor_three(test_visitor_three &&) = delete;
};


// Test binary visitation
struct lex_compare_visitor {
  int result = 0;

  template <typename T>
  void operator()(const char *, const T & t1, const T & t2) {
    if (!result) {
      if (t1 < t2) { result = -1; }
      else if (t1 > t2) { result = 1; }
    }
  }
};

template <typename T>
int struct_cmp(const T & t1, const T & t2) {
  lex_compare_visitor vis;
  visit_struct::apply_visitor(vis, t1, t2);
  return vis.result;
}

// debug_print

struct debug_printer {
  template <typename T>
  void operator()(const char * name, const T & t) const {
    std::cout << "  " << name << ": " << t << std::endl;
  }
};

template <typename T>
void debug_print(const T & t) {
  std::cout << "{\n";
  visit_struct::apply_visitor(debug_printer{}, t);
  std::cout << "}" << std::endl;
}

// tests

static_assert(std::is_same<decltype(visit_struct::get<0>(std::declval<test::foo>())), bool &&>::value, "");
static_assert(std::is_same<decltype(visit_struct::get<1>(std::declval<test::foo>())), int &&>::value, "");
static_assert(std::is_same<decltype(visit_struct::get<2>(std::declval<test::foo>())), float &&>::value, "");
static_assert(std::is_same<decltype(visit_struct::get_name<0, test::foo>()), const char (&)[2]>::value, "");
static_assert(std::is_same<decltype(visit_struct::get_name<1, test::foo>()), const char (&)[2]>::value, "");
static_assert(std::is_same<decltype(visit_struct::get_name<2, test::foo>()), const char (&)[2]>::value, "");
static_assert(visit_struct::get_pointer<0, test::foo>() == &test::foo::b, "");
static_assert(visit_struct::get_pointer<1, test::foo>() == &test::foo::i, "");
static_assert(visit_struct::get_pointer<2, test::foo>() == &test::foo::f, "");
static_assert(std::is_same<visit_struct::type_at<0, test::foo>, bool>::value, "");
static_assert(std::is_same<visit_struct::type_at<1, test::foo>, int>::value, "");
static_assert(std::is_same<visit_struct::type_at<2, test::foo>, float>::value, "");


int main() {
  std::cout << __FILE__ << std::endl;

  {
    test::foo s{ true, 5, 7.5f };

    debug_print(s);

    assert(visit_struct::get<0>(s) == true);
    assert(visit_struct::get<1>(s) == 5);
    assert(visit_struct::get<2>(s) == 7.5f);
    assert(visit_struct::get_name<0>(s) == std::string{"b"});
    assert(visit_struct::get_name<1>(s) == std::string{"i"});
    assert(visit_struct::get_name<2>(s) == std::string{"f"});
    assert(visit_struct::get_accessor<0>(s)(s) == visit_struct::get<0>(s));
    assert(visit_struct::get_accessor<1>(s)(s) == visit_struct::get<1>(s));
    assert(visit_struct::get_accessor<2>(s)(s) == visit_struct::get<2>(s));

    test_visitor_one vis;
    visit_struct::apply_visitor(vis, s);

    assert(vis.names.size() == 3);
    assert(vis.values.size() == 3);
    assert(vis.names[0] == "b");
    assert(vis.names[1] == "i");
    assert(vis.names[2] == "f");
    assert(vis.values[0] == 1);
    assert(vis.values[1] == 5);
    assert(vis.values[2] == 7.5);

    s.b = false;
    s.i = 19;
    s.f = -1.5f;

    visit_struct::for_each(s, vis);

    assert(vis.names.size() == 6);
    assert(vis.values.size() == 6);
    assert(vis.names[0] == "b");
    assert(vis.names[1] == "i");
    assert(vis.names[2] == "f");
    assert(vis.names[3] == "b");
    assert(vis.names[4] == "i");
    assert(vis.names[5] == "f");
    assert(vis.values[0] == 1);
    assert(vis.values[1] == 5);
    assert(vis.values[2] == 7.5);
    assert(vis.values[3] == 0);
    assert(vis.values[4] == 19);
    assert(vis.values[5] == -1.5);

    // test get_name
    assert(std::string("foo") == visit_struct::get_name(s));
    assert(std::string("foo") == visit_struct::get_name<test::foo>());
  }

  // Test move semantics
  {
    test::foo s{true, 0, 0};

    debug_print(s);

    test_visitor_three vis;

    visit_struct::apply_visitor(vis, s);
    assert(vis.result == 1);

    const auto & ref = s;
    visit_struct::apply_visitor(vis, ref);
    assert(vis.result == 2);

    visit_struct::apply_visitor(vis, std::move(s));
    assert(vis.result == 3);
  }

  // Test visitation without an instance
  {
    test_visitor_ptr vis;

    visit_struct::visit_pointers<test::foo>(vis);
    assert(vis.result.size() == 3u);
    assert(vis.result[0].first == "b");
    assert(vis.result[0].second == "bool");
    assert(vis.result[1].first == "i");
    assert(vis.result[1].second == "int");
    assert(vis.result[2].first == "f");
    assert(vis.result[2].second == "float");
  }

  {
    test_visitor_type vis;

    visit_struct::visit_types<test::foo>(vis);
    assert(vis.result.size() == 3u);
    assert(vis.result[0].first == "b");
    assert(vis.result[0].second == "bool");
    assert(vis.result[1].first == "i");
    assert(vis.result[1].second == "int");
    assert(vis.result[2].first == "f");
    assert(vis.result[2].second == "float");
  }

  {
    test_visitor_acc<test::foo> vis2;

    visit_struct::visit_accessors<test::foo>(vis2);
    auto & vis = vis2.internal;

    assert(vis.result.size() == 3u);
    assert(vis.result[0].first == "b");
    assert(vis.result[0].second == "bool");
    assert(vis.result[1].first == "i");
    assert(vis.result[1].second == "int");
    assert(vis.result[2].first == "f");
    assert(vis.result[2].second == "float");
  }

  // Test binary visitation
  {
    test::foo f1{true, 1, 1.5f};
    test::foo f2{true, 2, 10.0f};

    assert(0 == struct_cmp(f1, f1));
    assert(-1 == struct_cmp(f1, f2));
    assert(0 == struct_cmp(f2, f2));
    assert(1 == struct_cmp(f2, f1));

    f1.i = 3;

    assert(0 == struct_cmp(f1, f1));
    assert(1 == struct_cmp(f1, f2));
    assert(0 == struct_cmp(f2, f2));
    assert(-1 == struct_cmp(f2, f1));

    f1.i = 2;

    assert(0 == struct_cmp(f1, f1));
    assert(-1 == struct_cmp(f1, f2));
    assert(0 == struct_cmp(f2, f2));
    assert(1 == struct_cmp(f2, f1));
  }
}
