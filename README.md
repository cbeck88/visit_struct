# visit_struct

[![Build Status](https://travis-ci.org/cbeck88/visit_struct.svg?branch=master)](http://travis-ci.org/cbeck88/visit_struct)
[![Boost licensed](https://img.shields.io/badge/license-Boost-blue.svg)](./LICENSE)

A header-only library for providing **structure visitors** to C++11.

In C++ there is no built-in way to iterate over the members of a `struct` type.

Oftentimes, an application may contain several small "POD" datatypes, and one
would like to be able to easily serialize and deserialize, print them in debugging
info, and so on. Usually, the programmer has to write a bunch of boilerplate
for each one of these, listing the struct members over and over again.

(This is only the most obvious use of structure visitors.)

Naively one would like to be able to write something like:

```
  for (const auto & member : my_struct) {
    std::cerr << member.name << ": " << member.value << std::endl;
  }
```

However, this syntax can never be legal in C++, because when we iterate using a
for loop, the iterator has a fixed static type, and `member.value` similarly has
a fixed static type. But the struct member types must be allowed to vary.

The standard way to overcome issues like that (without taking a performance hit)
is to use the visitor pattern. One could hope that the following kind of syntax
would be legal:

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

However, our library can be used as a single-header, header-only library with no external dependencies.
The core `visit_struct.hpp` is in total about one hundred lines of code, depending on how you count,
and is fully functional on its own.

`boost::fusion` is fairly complex and also supports many other features like registering the
member functions. When you need more power, and you also need to support pre-C++11, that
is what you should use, but for some applications, `visit_struct` is all that you need.

**Note:** The macro `VISITABLE_STRUCT` must be used at filescope, an error will occur if it is
used within a namespace. You can simply include the namespaces as part of the type, e.g.

```
VISITABLE_STRUCT(foo::bar::baz, a, b, c);
```

## Integration with `boost::fusion`

`visit_struct` also has support code so that it can be used with "fusion-adapted structures".
That is, any structure that `boost::fusion` knows about, can also be used with `visit_struct`,
if you include the extra header `visit_struct_boost_fusion.hpp`.

## "Intrusive" Syntax

A third header is provided, `visit_struct_intrusive.hpp` which permits the following syntax:

```

struct my_type {
  BEGIN_VISITABLES(my_type);
  VISITABLE(int, a);
  VISITABLE(float, b);
  VISITABLE(std::string, c);
  END_VISITABLES;
};

```

This declares a structure which is essentially the same as

```
struct my_type {
  int a;
  float b;
  std::string c;
};
```

There are no additional data members defined within the type, although there are
some "secret" static declarations which are occurring. That's why it's "intrusive".
There is still no run-time overhead.

Each line above is a separate statement within the body of `my_type` and arbitrary other C++
declarations may appear between them.

```
struct my_type {

  int not_visitable;
  double not_visitable_either;

  BEGIN_VISITABLES(my_type);
  VISITABLE(int, a);
  VISITABLE(float, b);

  typedef std::pair<std::string, std::string> spair;

  VISITABLE(spair, p);

  void do_nothing() const { }

  VISITABLE(std::string, c);

  END_VISITABLES;
};

```

When `visit_struct::apply_visitor` is used, each member declared with `VISITABLE`
will be visited, in the order that they are declared.

The implementation of the "intrusive" version is actually very different from the
non-intrusive one. In the standard one, a trick with macros is used to iterate over
a list. In the intrusive one, actually templates are used to iterate over the list.
It's debateable which is preferable, however, because the second one is more DRY
(you don't have to repeat the field names), it seems less likely to give gross error
messages, but overall, the implementation of that one is trickier. The second one
also does not have the requirement that you jump down to filescope after declaring
your structure in order to declare it visitable. YMMV.

## Compatibility

**visit_struct** works with versions of gcc `>= 4.8.2` and versions of clang `>= 3.6`. It has been
tested with MSVC 2015 and it works there also. The "intrusive" syntax seems to compile fastest in MSVC,
based on experiments with the [online compiler|http://webcompiler.cloudapp.net/], I have no idea why
this might be however.

## Licensing and Distribution

`visit_struct` is available under the boost software license.
