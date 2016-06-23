#include <visit_struct/visit_struct_intrusive.hpp>

#include <cassert>
#include <iostream>
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

struct test_visitor_one {
  std::vector<std::string> names;
  std::vector<double> values;

  template <typename T>
  void operator()(const char * name, T v) {
    names.emplace_back(name);
    values.emplace_back(v);
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
};

// debug_print

struct debug_printer {
  template <typename T>
  void operator()(const char * name, const T & t) const {
    std::cerr << "  " << name << ": " << t << std::endl;
  }
};

template <typename T>
void debug_print(const T & t) {
  std::cerr << "{\n";
  visit_struct::apply_visitor(debug_printer{}, t);
  std::cerr << "}" << std::endl;
}

int main() {

  {
    test::foo s{ true, 5, 7.5f };

    debug_print(s);

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

    visit_struct::apply_visitor(vis, s);

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
  }

  // Test move semantics
  {
    test::foo s{};

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
}
