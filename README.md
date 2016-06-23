# visit_struct

[![Build Status](https://travis-ci.org/cbeck88/visit_struct.svg?branch=master)](http://travis-ci.org/cbeck88/visit_struct)
[![Boost licensed](https://img.shields.io/badge/license-Boost-blue.svg)](./LICENSE)

A single-header header-only library for providing **structure visitors** to C++11.

In C++ there is no built-in way to iterate over the members of a `struct` type.

Oftentimes, an application may contain several small "POD" datatypes, and one
would like to be able to easily serialize and deserialize, print them in debugging
info, and so on. Usually, the programmer has to write a bunch of boilerplate
for each one of these, listing the struct members over and over again.

Naively one would like to be able to write something like:

```
  for (const auto & member : my_struct) {
    std::cerr << member.name << ": " << member.value << std::endl;
  }
```

However, this syntax can never be legal in C++, because when we iterate using a
for loop, the iterator has a fixed static type, and `member.value` similarly has
a fixed static type. But the struct member types must be allowed to vary.

The standard way to overcome issues like that is to use the visitor pattern. One
could hope that the following kind of syntax would be legal:

```

struct debug_printer {
  template <typename T>
  void operator()(const char * name, const T & value) {
    std::cerr << name << ": " << value << std::endl;
  }
};


visit_struct::apply_visitor(debug_printer{}, my_struct);

```

This could be syntactically valid -- here `visit_struct::apply_visitor` is somewhat
like `boost::apply_visitor`, taking a binary visitor and applying it to each member
of the struct in sequence.

However, current versions of C++ lack reflection, which means that it's not possible
to obtain from a generic class type `T` the list of its members, using templates or
anything else, even if `T` is a complete type (in which case, the compiler obviously
knows its members).

This library permits the following syntax:

```

struct my_type {
  int a;
  float b;
  std::string c;
};

VISITABLE_STRUCT(my_type, a, b, c);




struct debug_printer {
  template <typename T>
  void operator()(const char * name, const T & value) {
    std::cerr << name << ": " << value << std::endl;
  }
}

void debug_print(const my_type & my_struct) {
  visit_struct::apply_visitor(debug_printer{}, my_struct);
}

```

It should be noted that there are already libraries that do this, like `boost::fusion`,
which does this and much much more.

However, our library is a single-header, header-only library with no external dependencies.
`visit_struct.hpp` is in total about one hundred lines of code, depending on how you count.

`boost::fusion` is fairly complex and also supports many other features like registering the
member functions. When you need more power, and you also need to support pre-C++11, that
is what you should use, but for some applications, `visit_struct` is all that you need.

## Integration with `boost`

`visit_struct` also has support code so that it can be used with "fusion-adapted structures".
That is, any structure that `boost::fusion` knows about, can also be used with `visit_struct`,
if you include the extra header `visit_struct_boost_fusion.hpp`.

## Licensing and Distribution

`visit_struct` is available under the boost software license.
