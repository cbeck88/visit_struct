#include <visit_struct/visit_struct_boost_hana.hpp>

struct scope_test {};
static_assert(std::is_same<::scope_test, scope_test>::value, "WTF");

#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

/***
 * Test structures
 */

namespace dummy {

struct test_struct_one {
  BOOST_HANA_DEFINE_STRUCT(test_struct_one,
    (int, a),
    (float, b),
    (std::string, c)
  );
};

} // end namespace dummy

using dummy::test_struct_one;

static_assert(visit_struct::traits::is_visitable<test_struct_one>::value, "");
static_assert(visit_struct::field_count<test_struct_one>() == 3, "");

struct test_struct_two {
  bool b;
  int i;
  double d;
  std::string s;
};

BOOST_HANA_ADAPT_STRUCT(test_struct_two, d, i, b);

static_assert(visit_struct::traits::is_visitable<test_struct_two>::value, "");
static_assert(visit_struct::field_count<test_struct_two>() == 3, "");

struct test_struct_three {
  int i1;
  const int i2;
};

BOOST_HANA_ADAPT_STRUCT(test_struct_three, i1, i2);

static_assert(visit_struct::traits::is_visitable<test_struct_three>::value, "");
static_assert(visit_struct::field_count<test_struct_three>() == 2, "");

/***
 * Test visitors
 */

using spair = std::pair<std::string, std::string>;

struct test_visitor_one {
  std::vector<spair> result;

  void operator()(const char * name, const std::string & s) {
    result.emplace_back(spair{std::string{name}, s});
  }

  template <typename T>
  void operator()(const char * name, const T & t) {
    result.emplace_back(spair{std::string{name}, std::to_string(t)});
  }
};



using ppair = std::pair<const char * , const void *>;

struct test_visitor_two {
  std::vector<ppair> result;

  template <typename T>
  void operator()(const char * name, const T & t) {
    result.emplace_back(ppair{name, static_cast<const void *>(&t)});
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

// Test types visitation
struct types_visitor {
  std::vector<std::string> result;

  void operator()(const char *, visit_struct::type_c<int>) {
    result.push_back("int");
  }

  void operator()(const char *, visit_struct::type_c<const int>) {
    result.push_back("const int");
  }

  void operator()(const char *, visit_struct::type_c<bool>) {
    result.push_back("bool");
  }

  void operator()(const char *, visit_struct::type_c<float>) {
    result.push_back("float");
  }

  void operator()(const char *, visit_struct::type_c<double>) {
    result.push_back("double");
  }

  void operator()(const char *, visit_struct::type_c<std::string>) {
    result.push_back("std::string");
  }
};

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

/***
 * tests
 */

static_assert(std::is_same<decltype(visit_struct::get<0>(std::declval<test_struct_one>())), int &&>::value, "");
static_assert(std::is_same<decltype(visit_struct::get<1>(std::declval<test_struct_one>())), float &&>::value, "");
static_assert(std::is_same<decltype(visit_struct::get<2>(std::declval<test_struct_one>())), std::string &&>::value, "");

static_assert(std::is_same<decltype(visit_struct::get_name<0, test_struct_one>()), const char *>::value, "");
static_assert(std::is_same<decltype(visit_struct::get_name<1, test_struct_one>()), const char *>::value, "");
static_assert(std::is_same<decltype(visit_struct::get_name<2, test_struct_one>()), const char *>::value, "");
// TODO: This should ideally return const char (&)[], is that easy to do within hana?

static_assert(std::is_same<visit_struct::type_at<0, test_struct_one>, int>::value, "");
static_assert(std::is_same<visit_struct::type_at<1, test_struct_one>, float>::value, "");
static_assert(std::is_same<visit_struct::type_at<2, test_struct_one>, std::string>::value, "");

static_assert(std::is_same<visit_struct::type_at<0, test_struct_two>, double>::value, "");
static_assert(std::is_same<visit_struct::type_at<1, test_struct_two>, int>::value, "");
static_assert(std::is_same<visit_struct::type_at<2, test_struct_two>, bool>::value, "");

static_assert(std::is_same<visit_struct::type_at<0, test_struct_three>, int>::value, "");
static_assert(std::is_same<visit_struct::type_at<1, test_struct_three>, const int>::value, "");

#include <boost/version.hpp>

int main() {
  std::cout << __FILE__ << std::endl;
  std::cout << "Boost version: "<< BOOST_LIB_VERSION << std::endl;

  {
    test_struct_one s{ 5, 7.5f, "asdf" };

    debug_print(s);

    assert(visit_struct::get<0>(s) == 5);
    assert(visit_struct::get<1>(s) == 7.5f);
    assert(visit_struct::get<2>(s) == "asdf");
    assert(visit_struct::get_name<0>(s) == std::string{"a"});
    assert(visit_struct::get_name<1>(s) == std::string{"b"});
    assert(visit_struct::get_name<2>(s) == std::string{"c"});
    assert(visit_struct::get_accessor<0>(s)(s) == visit_struct::get<0>(s));
    assert(visit_struct::get_accessor<1>(s)(s) == visit_struct::get<1>(s));
    assert(visit_struct::get_accessor<2>(s)(s) == visit_struct::get<2>(s));

    test_visitor_one vis1;
    visit_struct::apply_visitor(vis1, s);

    assert(vis1.result.size() == 3);
    assert(vis1.result[0].first == "a");
    assert(vis1.result[0].second == "5");
    assert(vis1.result[1].first == "b");
    assert(vis1.result[1].second == "7.500000");
    assert(vis1.result[2].first == "c");
    assert(vis1.result[2].second == "asdf");

    test_visitor_two vis2;
    visit_struct::apply_visitor(vis2, s);

    assert(vis2.result.size() == 3);
    assert(vis2.result[0].second == &s.a);
    assert(vis2.result[1].second == &s.b);
    assert(vis2.result[2].second == &s.c);


    test_struct_one t{ 0, 0.0f, "jkl" };

    debug_print(t);

    test_visitor_one vis3;
    visit_struct::apply_visitor(vis3, t);

    assert(vis3.result.size() == 3);
    assert(vis3.result[0].first == "a");
    assert(vis3.result[0].second == "0");
    assert(vis3.result[1].first == "b");
    assert(vis3.result[1].second == "0.000000");
    assert(vis3.result[2].first == "c");
    assert(vis3.result[2].second == "jkl");

    test_visitor_two vis4;
    visit_struct::apply_visitor(vis4, t);

    assert(vis4.result.size() == 3);
    assert(vis4.result[0].first == vis2.result[0].first);
    assert(vis4.result[0].second == &t.a);
    assert(vis4.result[1].first == vis2.result[1].first);
    assert(vis4.result[1].second == &t.b);
    assert(vis4.result[2].first == vis2.result[2].first);
    assert(vis4.result[2].second == &t.c);

  }

  {
    test_struct_two s{ false, 5, -1.0, "foo" };

    debug_print(s);

    test_visitor_one vis1;
    visit_struct::apply_visitor(vis1, s);

    assert(vis1.result.size() == 3);
    assert(vis1.result[0].first == std::string{"d"});
    assert(vis1.result[0].second == "-1.000000");
    assert(vis1.result[1].first == std::string{"i"});
    assert(vis1.result[1].second == "5");
    assert(vis1.result[2].first == std::string{"b"});
    assert(vis1.result[2].second == "0");

    test_visitor_two vis2;
    visit_struct::apply_visitor(vis2, s);

    assert(vis2.result.size() == 3);
    assert(vis2.result[0].second == &s.d);
    assert(vis2.result[1].second == &s.i);
    assert(vis2.result[2].second == &s.b);


    test_struct_two t{ true, -14, .75, "bar" };

    debug_print(t);

    test_visitor_one vis3;
    visit_struct::apply_visitor(vis3, t);

    assert(vis3.result.size() == 3);
    assert(vis3.result[0].first == std::string{"d"});
    assert(vis3.result[0].second == "0.750000");
    assert(vis3.result[1].first == std::string{"i"});
    assert(vis3.result[1].second == "-14");
    assert(vis3.result[2].first == std::string{"b"});
    assert(vis3.result[2].second == "1");

    test_visitor_two vis4;
    visit_struct::apply_visitor(vis4, t);

    assert(vis4.result.size() == 3);
    assert(vis4.result[0].first == vis2.result[0].first);
    assert(vis4.result[0].second == &t.d);
    assert(vis4.result[1].first == vis2.result[1].first);
    assert(vis4.result[1].second == &t.i);
    assert(vis4.result[2].first == vis2.result[2].first);
    assert(vis4.result[2].second == &t.b);
  }

  // Test move semantics
  {
    test_struct_one s{0, 0, ""};

    test_visitor_three vis;

    visit_struct::apply_visitor(vis, s);
    assert(vis.result == 1);

    const auto & ref = s;
    visit_struct::apply_visitor(vis, ref);
    assert(vis.result == 2);

    visit_struct::apply_visitor(vis, std::move(s));
    assert(vis.result == 3);
  }

  // Test binary visitation
  {
    test_struct_one f1{10, 7.5f, "a"};
    test_struct_one f2{11, 7.5f, "b"};

    assert(0 == struct_cmp(f1, f1));
    assert(-1 == struct_cmp(f1, f2));
    assert(0 == struct_cmp(f2, f2));
    assert(1 == struct_cmp(f2, f1));

    f1.a = 13;

    assert(0 == struct_cmp(f1, f1));
    assert(1 == struct_cmp(f1, f2));
    assert(0 == struct_cmp(f2, f2));
    assert(-1 == struct_cmp(f2, f1));

    f1.a = 11;

    assert(0 == struct_cmp(f1, f1));
    assert(-1 == struct_cmp(f1, f2));
    assert(0 == struct_cmp(f2, f2));
    assert(1 == struct_cmp(f2, f1));
  }

  // Test types visitation
  {
    types_visitor vis;
    visit_struct::visit_types<dummy::test_struct_one>(vis);
    assert(vis.result.size() == 3);
    assert(vis.result[0] == "int");
    assert(vis.result[1] == "float");
    assert(vis.result[2] == "std::string");
  }

  {
    types_visitor vis;
    visit_struct::visit_types<test_struct_two>(vis);
    assert(vis.result.size() == 3);
    assert(vis.result[0] == "double");
    assert(vis.result[1] == "int");
    assert(vis.result[2] == "bool");
  }

  {
    types_visitor vis;
    visit_struct::visit_types<test_struct_three>(vis);
    assert(vis.result.size() == 2);
    assert(vis.result[0] == "int");
    assert(vis.result[1] == "const int");
  }
}
