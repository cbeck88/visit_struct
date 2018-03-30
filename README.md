# visit_struct

[![Build Status](https://travis-ci.org/cbeck88/visit_struct.svg?branch=master)](http://travis-ci.org/cbeck88/visit_struct)
[![Appveyor status](https://ci.appveyor.com/api/projects/status/6ksqg7es938cttn2/branch/master?svg=true)](https://ci.appveyor.com/project/cbeck88/visit_struct)
[![Boost licensed](https://img.shields.io/badge/license-Boost-blue.svg)](./LICENSE)

A header-only library providing **structure visitors** for C++11 and C++14.

## Motivation

In C++ there is no built-in way to iterate over the members of a `struct` type.

Oftentimes, an application may contain several small "POD" datatypes, and one
would like to be able to easily serialize and deserialize, print them in debugging
info, and so on. Usually, the programmer has to write a bunch of boilerplate
for each one of these, listing the struct members over and over again.

(This is only the most obvious use of structure visitors.)

Naively one would like to be able to write something like:

```c++
for (const auto & member : my_struct) {
  std::cerr << member.name << ": " << member.value << std::endl;
}
```

However, this syntax can never be legal in C++, because when we iterate using a
for loop, the iterator has a fixed static type, and `member.value` similarly has
a fixed static type. But the struct member types must be allowed to vary.

## Visitors

The usual way to overcome issues like that (without taking a performance hit)
is to use the *visitor pattern*. For our purposes, a *visitor* is a generic callable
object. Suppose our struct looks like this:

```c++
struct my_type {
  int a;
  float b;
  std::string c;
};
```

and suppose we had a function like this, which calls the visitor `v` once for
each member of the struct:

```c++
template <typename V>
void visit(const my_type & my_struct, V && v) {
  v("a", my_struct.a);
  v("b", my_struct.b);
  v("c", my_struct.c);
}
```

(For comparison, see also the function `boost::apply_visitor` from the `boost::variant` library,
which similarly applies a visitor to the value stored within a variant.)

Then we can "simulate" the for-loop that we wanted to write in a variety of ways. For instance, we can
make a template function out of the body
of the for-loop and use that as a visitor.

```c++
template <typename T>
void log_func(const char * name, const T & value) {
  std::cerr << name << ": " << value << std::endl;
}

visit(my_struct, log_func);
```

Using a template function here means that even though a struct may contain several different types, the compiler
figures out which function to call at compile-time, and we don't do any run-time polymorphism -- the whole call can
often be inlined.

Basically we are solving the original problem in a very exact way -- there is no longer an explicit
iterator, and each time the "loop body" can be instantiated with different types as needed.

If the loop has internal state or "output", we can use a function object (an object which overloads `operator()`) as the visitor,
and collect the state in its members. Also in C++14 we have generic lambdas, which sometimes makes all this very terse.

Additionally, while making a visitor is sometimes more verbose than you'd like, it has an added benefit that generic visitors can
be used and reused many times. Often, when doing things like logging or serialization, you don't want each struct to get a different
implementation or policy, you want to reuse the same code for all of them.

## Reflection

So, if we have a template function `visit` for our struct, it may let us simplify code and promote code reuse.

However, that means we still have to actually define `visit` for every struct we want to use it
with, and possibly several versions of it, taking `const my_type &`, `my_type &`, `my_type &&`, and so on.
That's also quite a bit of repetitive code, and the whole point of this is to reduce repetition.

Again, ideally we would be able to do something totally generic, like,

```c++
template <typename S, typename V>
void for_each(S && s, V && v) {
  // Insert magic here...
  for (auto && member : s) {
    v(member.name, member.value);
  }
}
```

where both the visitor and struct are template parameters, and use this to visit the members of any struct.

Unfortunately, current versions of C++ *lack reflection*. It's not possible
to *programmatically inspect* the list of members of a generic class type `S`, using templates or
anything else standard, even if `S` is a complete type (in which case, the compiler obviously
knows its members). If we're lucky we might get something like this in C++20, but right
now there's no way to actually implement the fully generic `for_each`.

This means that any implementation of `for_each` requires some help, usually in the form of *registration macros*
or similar.

## Overview

This library permits the following syntax in a C++11 program:

```c++
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
};

void debug_print(const my_type & my_struct) {
  visit_struct::for_each(my_struct, debug_printer{});
}
```

Intuitively, you can think that the macro `VISITABLE_STRUCT` is defining overloads of `visit_struct::for_each`
for your structure.

In C++14 this can be made more succinct using a lambda:

```c++
void debug_print(const my_type & my_struct) {
  visit_struct::for_each(my_struct,
    [](const char * name, const auto & value) {
      std::cerr << name << ": " << value << std::endl;
    });
}
```

These two things, the macro `VISITABLE_STRUCT` and the function `visit_struct::for_each`,
represent the most important functionality of the library.

A nice feature of `visit_struct` is that `for_each` always respects the
C++11 value category of it's arguments.
That is, if `my_struct` is a const l-value reference, non-const l-value reference, or r-value
reference, then `for_each` will pass each of the fields to the visitor correspondingly,
and the visitor is also forwarded properly.

It should be noted that there are already libraries that permit iterating over a structure like
this, such as `boost::fusion`, which does this and much more. Or `boost::hana`, which is like
a modern successor to `boost::fusion` which takes advantage of C++14.

However, our library can be used as a single-header, header-only library with no external dependencies.
The core `visit_struct.hpp` is in total about four hundred lines of code, depending on how you count,
and is fully functional on its own. For some applications, `visit_struct` is all that you need.

Additionally, the syntax for doing these kind of visitations is (IMO) a little nicer than in `fusion`
or `hana`. And `visit_struct` has much better compiler support right now than `hana`. `hana` requires
a high level of conformance to C++14. It only supports `gcc-6` and up for instance, and doesn't work with
any versions of MSVC. (Its support on `clang` is quite good.) `visit_struct` can be used with
many "first generation C++11 compilers" that are now quite old, like `gcc-4.8` and MSVC 2013.

**Note:** The macro `VISITABLE_STRUCT` must be used at filescope, an error will occur if it is
used within a namespace. You can simply include the namespaces as part of the type, e.g.

```c++
VISITABLE_STRUCT(foo::bar::baz, a, b, c);
```

## Compatibility with `boost::fusion`

**visit_struct** also has support code so that it can be used with "fusion-adapted structures".
That is, any structure that `boost::fusion` knows about, can also be used with `visit_struct::for_each`,
if you include the extra header.  

`#include <visit_struct/visit_struct_boost_fusion.hpp>`

This compatability header means that you don't have to register a struct once with `fusion` and once with `visit_struct`.
It may help if you are migrating from one library to the other.

## Compatiblity with `boost::hana`

**visit_struct** also has a similar compatibility header for `boost::hana`.  

`#include <visit_struct/visit_struct_boost_hana.hpp>`

## "Intrusive" Syntax

A drawback of the basic syntax is that you have to repeat the field member names.

This introduces a maintenance burden: What if someone adds a field member and doesn't update
the list?

1. It is possible to write a static assertion that all of the members are registered, by comparing
   sizeof the struct with what it should be given the known registered members. (See [test_fully_visitable.cpp](./test_fully_visitable.cpp) )
2. It may be *useful* to register only a subset of the field members for serialization.
3. It may be a requirement for you that you cannot change the header where the struct is defined, and you still want to visit it, so the
   first syntax may be pretty much the only option for you.

However, none of these changes the fact that with the first syntax, you have to write the names twice.

If visit_struct were e.g. a clang plugin instead of a header-only library, then perhaps we could make the syntax look like this:

```c++
struct my_type {
  __attribute__("visitable") int a;
  __attribute__("visitable") float b;
  __attribute__("visitable") std::string c;
};

void debug_print(const my_type & my_struct) {
  __builtin_visit_struct(my_struct,
    [](const char * name, const auto & member) {
      std::cout << name << ": " << member << std::endl;
    });
}
```

We don't offer a clang plugin like this, but we do offer an additional header,
 `visit_struct_intrusive.hpp` which uses macros to get pretty close to this syntax, and which is portable:

```c++
struct my_type {
  BEGIN_VISITABLES(my_type);
  VISITABLE(int, a);
  VISITABLE(float, b);
  VISITABLE(std::string, c);
  END_VISITABLES;
};
```

This declares a structure which is essentially the same as

```c++
struct my_type {
  int a;
  float b;
  std::string c;
};
```

There are no additional data members defined within the type, although there are
some "secret" static declarations which are occurring. (Basically, a bunch of typedef's.)
That's why it's "intrusive". There is still no run-time overhead.

Each line above expands to a separate series of declarations within the body of `my_type`, and arbitrary other C++
declarations may appear between them.

```c++
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

When `visit_struct::for_each` is used, each member declared with `VISITABLE`
will be visited, in the order that they are declared.

The benefits of this version are that, you don't need to type all the member
names twice, and you don't need to jump out of your namespaces back to filescope
in order to register a struct. The main drawbacks are that this is still somewhat
verbose, the implementation is a bit more complicated, and this one may not be
useful in some cases, like if the struct you want to visit belongs to some other
project and you can't change its definition.


## Binary Vistation

**visit_struct** also supports visiting two instances of the same struct type at once.

For instance, the function call

```c++
visit_struct::for_each(s1, s2, v);
```

is similar to

```c++
v("a", s1.a, s2.a);
v("b", s1.b, s2.b);
v("c", s1.c, s2.c);
```

This is useful for implementing generic equality and comparison operators for visitable
structures, for instance. Here's an example of a generic function `struct_eq` which
compares any two visitable structures for equality using `operator ==` on each field,
and which short-circuits properly.

```c++
struct eq_visitor {
  bool result = true;

  template <typename T>
  void operator()(const char *, const T & t1, const T & t2) {
    result = result && (t1 == t2);
  }
};

template <typename T>
bool struct_eq(const T & t1, const T & t2) {
  eq_visitor vis;
  visit_struct::for_each(t1, t2, vis);
  return vis.result;
}
```

On clang 3.5 with a simple example, this compiles the same assembly as a hand-rolled equality operator. See it on [godbolt compiler explorer](https://gcc.godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAKxAEZSBnVAV2OUxAHIB6bgaj4QAwgEo%2BQ1AAcAnsTzAEBPgCYADLQCsfALQr1AdnEI5DIpISZifAEKZkAawCkqgILOXvAQBE8puQCNmAkx0PmYAOywrAgsbVFRTPgBlVAAzAgB3AENiTD4AGTx2cIZMUj4ANUsGPFRwvloAOlVGwSTMPKzkNABbSSzw6Txw4HdPPlS8ABs8/IBJIWwAOSTsPhI%2BNBk%2BLKVFAkkQXgyTxv9400aSYG55xZXsAH1aR5aCAA8CEXcf5QBmPCpSKYVKVOZJOYAFUeSUhACUAKpCaEACQACmjHnMlkJ8givNgvL8/lhJuE8hVwVCYfCkaiMVicXiCUTXGMAFSc9x8dl8NHEVAANzwWAYOwmXWmhGkfCIfCwyCmOU6fD8zGQBFYnTFjmUymFNQIWX8M11yh2kR2kkkUxlWT4BsIJG5vLlhFakNiU18SjSfB6mB6/mqfF84t6kmmmG0BDwAflu3tao1WvKA1CMUseTD4VQLr4xAiMbjeSFlgsWXQjRd3B%2BbP%2Bw0VzCwfEcfyEQSlBGkbewxMbU2beTbQm7kkwjwIxCyhAYvbrHn4ADENgBZJIVIR6Wh/VXMa0kAjlDJ5SRBTZ1UyYd6SKzBhDDUL20nDTDVtn13eA4GgykQ6GwoiyKPEIADyKyQtgAAaaJwsSAihqCEAvuSoSPOuQiPBU2BwmIuoAGwEYI6FJJh2Fwq27Ztl4DQAByqKo3yfghAgoRSVIAbSwFgRB0GwfBAiYFMpQCQhbFgv%2BNJAdCPGwnxFFoCUwQ3sQomYJEgLwep%2BCpAunguOkljiLq1gmbQAAssqxFk1q2o8jpEFYqQRBqtT1MgAx8MGF5Kdet7voufAAOp5EaxDAJgShCCZZn0F554MOqCATC5saXjsuSylkwARaEf7UoBdIgVBUEWSB4FyTBcIBfB36khJBVcTJJVlbJkFVaJgKCMhIKvmhGFYTheHKIRI3EQN5GUdFfw0bQACcDEiHh%2BjRStghgGAbFoY8yA2swDB7Qwy2rVuECPDth2HZRRIzdutCqH8%2BRMW4/wsfKvXkg1nHScVpXmeVvEdcxgnCZgomsR97GSYV3Etf9bXyT5V4qWpGm6cx2maR%2BLjhFkAYHV0eQOY8yZKI4%2BimdjuP4/07CytOs6thTen8PycY5DKwR9EqwR8BkD7IMlYYHXYeBZN6ABeISyqgBaYMAPpGfaY5g64XM2rsw7tir1N5JC5Q63jeTLC41j5Gs1EOqgIrzq4pMOr4hDGqafyU24rieCiQnjtEgYa7z/NFMlgt2PYYpdcr0jjqGYq5ArV65Og7jqzzWujlH6lG3w%2Buyhnut8CbZsW7dgrW0nfx9nbU7qkovj2Y7RomnkICqgQ6AgCAqTi6Uk4Z0zpn6DdbvJ37qdTYb8aQrbLj23XDnO2D7b69yb2r2vufjvnpjtyAmdN48mnawzBAMB389NyOU8V2f4vMIv2Adyr098C32%2BP4WE4q/35NDyzfBe1MH2spR6az5gLZKuQehljFMgQUFpQi5FSFmcIdNUgCh6OKL%2BEAGBZEFJgMUTB4xjmGMAZ6KdQEjgnnrae9tFSYE8uTN2AgVb1Soa3HekCyw7UFJQvOWc34gE4Xgx4iDkHsEvr2R%2BGdJEgC/k/V2PxB5tmHmrEBvNeGbyzlfSuLh9okM2DMAYk5rob0zvGOhAwJHX1kdIhR2NPCrmGHgbQeDiA1DqOsUEAjehQPCL3aOEATzrHCLaOWABHZgeBMrRWUKZWJtBaBkLUWnNhOc2EIhodXDUF4ei%2BP8cOZmrhmEZ3qgqKYKsIDVy1kudhHcymCnFlY7AEAxCvzbnUuwUwGlTBHBkiuLSxDyOHkoux7s3CDz4J4bSfBdYEzplOGcJ8/6QmyhMDYRpgDyl8P0AggsR7cwodrPhk9MmFmySrHa/cwg1BGKYkxV9rBM1/vY/gLhuj4KYNEDOLdnIoLSvUVA/gqB2CUOpDykhEo830faSQ1twjBGIDGVA2gAxBksPs/2KTjl5FXIGNEU5yi4p6PiqwkgpynJrjsd5DBPnfyKcAg56ijmaJORXFe%2BVvpFQRlVHYQRZZSEsLsEgLSICQlbCNIiXwkZKG0L2d6ioKkCNSCQbIxBy7Il7JUkQjR2RkuIMtJhCFciamIPUJVKqcjqu0VqnVerlErx/oo55YzeBjH4AiUoiLhgIq7uIj2tYXl8DhJFVg9RwjMDRVYP0kwhLoHDvUe058ZitzOQQDFY8NFmLyEkaeHKpJcoqu1WCtSQA1ClsYmNUx0A7RYPClpijDXGtDfTRZp8QBJsXqOY%2BbaLF%2BLTe2HNbLrGVurWgCI/bhmsjGeQpl6cWXZtzRxfN3FC2IwEWWz%2BEw8CxpreOiASRxWjXwgauWJr6gjt3fCkcg7mnfFdk8hcbqrQ2mkPXQ0GwIB1DyLPJSAx2BJMZVi%2BdyQDbYsqIu6GTVfqtVXTyrIfLn12QcsKioh6iKCnKAegiREjo6GfqYre7Td6433ofaK9L14IQWbODuc8G4L0od2juvbJzXvnNY7pd8V6UZkU/NkhSXDMKY%2B2%2BjF8j6tuY4YvtbGh0PxADZF92CiPKuIKq9VFRNWChEOUc1qnLUyeaUdO9k6/4KaQw3D9mRZY/tMH%2B/BAHMXjzA0kOKbCkjKFA8BjTbLXB5phs1P6ANKrFvg3KMzr7kPEAgKh7DY0MPJFoGhsaDA4ruaS/hVUygxCyp84Jgj/CiN7xmAfXSVEKOUZbTRkAdHDQMbK3lirVHhMsf7XOrNlWT7MdQLkuo%2BTr2peULx2xOjGsCBkZx1WDX15DfHA2leejbk%2BN61h26bDqOdZQN1vJ8ihAucw4N6xQyV7rbbR2xjEmUBSdY%2B2Jbfib0yPC0pneKm1Mjm880rTpBuOjZ%2B790bunXsDtoJqlL2nvt/Yh5DgQAP9MDoO4ZrLxmnV/xU48ehgtBAuHKZYXGvMGDSHhVkd4ayrDhbfU6MgctXGlDFCQKIniMrAAjepE%2BDmM3MvaxUTz7X7u%2BaXf56D8NYMhYQ6j9HCB93pdVOUGLEq4vZfw2wgRRWJxkfB%2BvE7tGGDk8bi7LtF2WsGfG7fSbjWZuTcYcd4TZ3xNVcNwO9jcnHsw7VW9zT2mS0vdh7tkHS17X8anU%2BsXXRkoZEIMlKzKaa5alDL%2BlB%2BD02HLa/nPb%2BX4zue5/nd77g/NQaEHDILRaKKhdlsHwW%2B7EuxYyyl/bUuGAecqFLrTeHcvFOA8rkjxW1flco5r6r2ubfkam41vv9vk9Z1H1t3rO3U/ufN/hs3HGTfq7evP/jhqFubNuzCExa3mtT77bY3bA35%2BjKExdwfo%2Brute3/d6xzvlMWtd%2B2d7EBPsr6h5/irLv1UuZB4kr7HvL/YA9eH/a9eHbBRHf3cZQPf1SoBuKyUKDOMUN/BuEmLJAgKRccHaEcRoPA3sMQP0TMOWeOBFaWVFYMNxRPWdNzTPLObPPnSDH6fPQLblEXOUYmFWBgaLJvBXVvNPb9QrTvVXUrIfH7PvGrJ2MTfXO3a/I3JfQcU3CrNfcZQ1PvS/ZrOQh3WTM%2BNArgp7DuL3Z/IQV/LTJHAPP%2BCoBA4gigoyWFb1EMCAAiJIDuLIQg0EYguORWROf0QMSgucVRQDJzYDJIOg%2BMBglwXPZggvNg4vBDYmew%2BFaoHgqvB0PgkbAQktFXErEcD/N6CQgfUTPXK/ehaTbQvsBQrjIA1fQ7aRdfK3C/IoztEooxVrO/OTBIuFBFbgsAl/d3cwmAyw6w6yalT5FA35VyDxQFYFDUXDIg2ILwhOcgvw6oagoDHnMIikCDRqaI1g4XOIjgtArodgGlEgbg2XI9NIlvDIpXIQheHI%2BrcQ4TSQ3XZozQ0o67H3HQ9tZfaoliFQy3eldQpo87WQj4tox3XQw0R4Y4j5M4gwzuJ/dTfo6Ax1bGJ9FwWyGUSLQQMPGIGZWWYYWzePI6J9T0MMDyKYYSB2aExI7o8oFTLyLoewNTGBbrHZOWLIJgEoNY4IjYzIiIqIrlGI/Y3lMLLEnXFDXg6447MDDve47vYfDXZ4wo2raQlosor4iojo34pUmouTPjVQleYmUmKEwgR4Ok1Y8ohEow5E/pMw1EpRdEuAgAcUigdBNy8hlEfGvEEG9HsEEJ3giiUEZM1BtHs15JHG9VDHQHeE2OSG2M5RXUBnYNlmDMl1SNwxyxuLlLuNI1ELyJYgKJ1zqxkI2zH3aJvkUMAL1NXjKQqWBLVOKPeNaPkLk2DPshNwRIcOAGnCmBrTj1a29XKBFHeHnGZkHh00fz02MJvWwSWmekXwNLqKNPpSbVNQ61OxBNt3LK0K1JkQ7Im27KSN7PFgHOJKHPhRHNjPHIHi8CnOeyRIM3nIGLRJdX4DdKUHzmjW3SrXKH8G9OBDHMCMcyjPhRjLjMyN50iP5zz1FJLz4A7N1haRlKBNzJ3myMVKeIuxeNLI1M%2BMrJ%2BOrMLLEk6QbOt23LLJ7T3MIqQqNmPOCFPP7MUgvLAqPAgtvJ/j9z%2BIQgBIEwEHXPqEbKkObINxoshJADooDAYvlj7PPKNCvXbGHI4rZQnK8FfKdOnWSSmmjNHPjOgqFOTOCwOLTMikeGQqwzl2PWuPlXKQzmgDMt1ijNjMw01SWjpTy0EsQscqNmcsgrnI0tgMCk/N8MjT4EtMpwAtjywGApcBnTTl0pcqgsTOXRkngoQw7IipQuzNlPbzzK7wLJ4qLJVJLPVJbM1MIomxrJ%2B3rPsuEteNBN3PBLbI7kyq6MsBkqYvkoGEvPYtHM4sHm4trN4tqNm3qLXJDQ3PqrwvKoIokraocKiwER7LkpYoUt6uvLHNUrvMCoXHip0vAr0uSty0MrSpTJMu8oIAtPaqi0sqPXSNsoqQWqSNUiUqSoCoNRXi8ueoRT8tcv6V2sDRCtsNJ1GJIH/MApisjKUsOqSrcxSoF1iLFNMquthNOKi3SNyva3lPzNyKKqaxwtVJEreLEuavKON2IvxohgVTqooqbJJrBNbPJusQ7LRs%2BU6tWsvHWrYs2oGvUsXOUNGot34tPWbWmrKtJqZv3JZrMrZuFWWpPM5sHJ5pUr7DUsBq0qCLYogv0oRrgvOuRsuphLBtuvSwetqvHActRpNr%2BoTIBs%2BomrPSNrlteqECOoCsdKCs8BCpVghuiuvGhrdthsgvhty3tguV2BJg8oEE31MUuUtgtswEqTpuJsauorJuluXOwN2A5rPLWp6pVv6u2q4oGJjpuU2S/ktjWwzh2iwMXhM1GUDsSpDuc2nljojrJlWzAw7pJltpvTrtRNdPdO/K8QwK1EDtDp0VOsL0RgQqkqTsxrQryowuEIeLENG2LI0MloqokqqpIuprsstvFtEsZp3u%2BPnoGRIr4sbUmqEpToap3PTqltop8uko1ontbpOtguYPSrlAvruoInNrIvsvnufPcsBM8tvqNqcutPfo9k5HZHzEhEJO5kDBZ0QK%2BlNnNlSv9C6AFGPDyHJGlntB6BshRTwdQGPHAU2E8iNADPzGIJHvFFIeQAFHgVVG61LEzFJ3CmZ3hQYDTEtHCysl2FwdYdljlHF3WG4cZz4YIACh5CQYsEymVA4aITsAQHCDwAiXwT4GkBYBofqH2jyDxIfHqC3FvEwFvFQBONpTC26HZO9AYAjwfAIXVGQEbAIFtGqt5CitKAIAIFuX3HFWUFyGQFYBqDwTNDEYFAYAUcQaKV5GsHPBiApM8m7llg8mMdDCUD/rwDwT5jOLyEsAFCsHxhwQijFH5nUjUcijMc2WAFQF0YyAFBGHifzBCgykIdQAyHlDhU2UIFj2YZIGHD1HQAjX8DNH/OYCpMimsb9GyBlEDgxwQC5IwcrAaUJwihEdrlrgCME15CgUylhWIFePYYVjwTFGDACaMhKY2HKeynwXib4BXCsHwAYHCZpTclVA6D4DmFXDRHNlXGWEhBcEhDmHAkeCWFAkgiSEaB6CTkSaUbyDQBbGDCmB6ZjiZNKFCA8RhV2AxzQW6z4AACkcgMBzIG90wsXcxemIpyRpxghQgoqYVpAYgPEPm5AyVnnPRTxWBYVSgGdiD6XBUmWLwWwww5RxJp6GRVwXA0QdgxRRRWG8Bgwqwaw9IEG%2BA3SGXNZQhUW8gEGA0xlbNYxkAvpUqZ6eU1rY8lBSH3hSrisQadRbp8I5pB6XoSRIYLWBc%2BIXAlgvAIB3gxBYrdQvWyQoYdiioGQXA4QXSoWIBWwNB17V5nhyhHgG9Hg/h03zJ02NB038J039B03aJ025p037ovtk2P9ng4pnhM2dwK3c2%2BBnh82W3aBC323i323S323y2W21Aq2U23oM2629R03/gJ3m2M222M3O2M3u2M3e2M3%2B2s3VAh2a2/g62/hM2/hs2W2/hp2/hZ2/h52/hF2/hl2/hV3zJ13HBq2/jHhzI63KWc392n3p3zJZ3zJ53zJF3zJl3zJV2NA72H3azHgNA62NBM2NB32NBp2NBZ2NB52NBF2NBl2NBV38JQPh2WJHh8I638JM38J338Jp38JZ38J538JF38Jl23Xyglhyg8DGgxAlhiRpXv7o3MQlhY2XSIAWOxBp6/WA3osuPgIY242E3zoKgXBHg%2BOkhzpygk3cOEIGO%2BA6PygaOtPO2KOtPm2SOtOG8COtP12xswP15MPyh0PrPu3kPrO234PrP93oPrO4oQPlP73VOBAgPygAO/Pu2f2/O23zJm3zJ93X2%2BBn2/OzOVOP9r3yhL3Evu3T3Eu23D3Ev92d3Eu4o/hYuvOP9lB%2B3lBe3lBu2RpyhlA23lBm3J2VAG9lA4pB3zPvOGh%2B3aBe3aBu2O3yhNBevm3G2GgG8Elev8uLO15%2B3e3u3O2QvEvKvevyhGJnoOQZkOhQhrwFk%2BAZg8EpgZYSc6YNvpw%2BBMBukeRjWw3OOmCC0cQwXHgIAshSB/AxB7Qw2w2vIOPvXp6eIhAwX7vHuhPxOzqfvoQ/unvH1/hLuo3gJ5XAWABNJcVQCAVIZbiHz7wH%2BTjEfIeH2gJH0gZ4MQVIM6RJD7iNn1qDGHrHpcZQXHtNjNgnon%2Bnunkn18Mnn6Cn%2BHv4Gn%2BgDNvHv4RnxJCYM6LLQXrNlH8Nln6e9npccyLnvHjzLNvH8yfnxn4Xwn0XkXpX5nz6SXzH%2BHjQWXnnhXp9vHjQZXkX1Xs6PnjXxn03rXyNpM6EKX/CA3%2BX7N43iDvH6ytXgXtXi39XtXpXkX03kX49O31noqKX/QF33nxXk3z3vH/QM333xnq3gPm3xnr3s6RPsPnXuHpcWiaPo33Nj3/DhPvH2iJPoXlPxnwPtX4PtXzPx4RPkXivnP9HqXuaQvt34v/N0vpv8vvHuaSvunkX1Ps6Wvs6evs6Rv5vtXivkXoftvq76H3Xpce6Lv2Pkvwt/vx4Utx4ct54RiEXn3qv0fmv9PkPxn2fs6eftXof4/xiJfqHx31fhJDf933v7f4t3fwfvHyt2t4fn7zH5PsL%2BDfK/oz1v5nR7%2B3vI/t72J6fhIeDvDHnn1oDU9Ugf/OXjHw/7x8d%2Be/A/v/xG71tAB1fa3kHwz7gCW%2BjPaAUT1gFE8T%2BRAp/kgKl47h3%2BPfHAd/zwF/912tbP/vLx3DECz%2BpAuvuQJF7X9d%2BVA/nrQIAHH8/e/AxgZa2YEy90BtPV3pv0/5l8f%2B%2B/LgX/256oC/%2BbvCyAILV7ACJ%2BEHEQWrzEGQD9%2Bkg/nvQNQH89gBhg%2BQQLmYH69lB3PVQdgL74cDf%2Bh/HQbwP0F/9i%2BmgIwZb3P5kDL%2BogiARIIf62D%2BesgxwaYJCHODyer/Z3u4MwFF84%2B3ggfloL8E8D62gQ54MEN74dtQh/vcfqAOn4UC5%2BMQmAXEJkEOD%2BeSQqfs8FD4IC0ey/F/igKj4ZDDe3fbIV/1yH4DuBhAvQc8AMElC/%2B2/LruUJMFVD8ONQm/nUJoENDveCQ5ofz1aFlDj%2B2fDoaT1z6U8OurAwYRoM4H5CxhfAyYX/1KEzDv%2BHXOYeEOEGRCLB0QhfjYOP52CNhx/Fofz0b6zDj%2BrffYRL3b6v9O%2BfQzwWwJyGaCRh/gwoRMKCE3Dphf/e4Xv3miPChBk/cwVnzeF38PhcA%2BIU0J%2BFbC/h/PMQQ8OP6L9gR2vUEXnzUAnCt%2BZw3wQQN0FXDERrbZEc8FRF/8D%2BagDEWnwiFgCohlA94bEM%2BGEjj%2BiQkkcf3%2BHkirB6I83o/2pH28FBq/RrgyPUG4DmRow1kUUIshIi2hKIv/miN5HcDGu/IyoYKOqHCjahoo%2BoeKMaGSjNhx/bYbKP57yjqBGbKQeaJSFs81RaAjAf0LUHsDhh2ggoeMMbbFCDRPXLkcaJ5Fy8zRHg2QSrxIECjnhQo14SKLxFiiCRjo73lKJdGkjdh7o/np6L5Hm96BeoMXogNVF0jOeEIrAVCKGEwiwxlwvUVMMNGxjngJohMXLyTFy9HBKYwQWmKxEvCcRWYqAfiLoESj8xzo73q6LJEljKRKvb0ZWL97/BqxnQ5/sgMp41cNRIYlsRcN1EIioxHIzsV1zjHPBTRfYuXvL0nYZskhQ44wU8NHEZjxxto7MfaNzHrCiR3vX4TKMXGAjSxK4lXmuJV7ACaum4g4bSN3FuDAxkI04VqLyEsiAhJ4/UWeJjEXjux8Yr0TeIzZ3i3eNXOXtsKfFhDMRZgscU31xGTicx04vMUTwLHziix3vOUcBMVGgSVe64iCaYKq5QSQRXQncfDxGj7joR5wlCfCMjHoS%2Bu547kVeN7GjtbxA4uXsXyq5y9/hpEioSAKtGLCbRywu0asIdE/inRxIwsQBOLFATlx7EisZxPAkq8eJrQkaHxJpECSpeZXESc2LEk6jUJkkjsZhNknzR5JjXRSRm0IkqTe%2BFXBdhaK0npjrRmYj8TRK/F0SjJs4kyUxLMksSlx3vMsauJsnm9uJKvByY3zK5OSVRLgtUQXwbFZDGRSE2EeGLZGnjpJfky8QFNwkKT8JSkh8cRLUly9v%2BJXKKaYNaEz9qJ1g2idIOSkMS5xRPBceZO94eiQJ1k83lxLskFSVeRUsQX1N9ER81R4I%2BCY2MQk%2BDkJXkiSdcIwl3Dmp14tqeOxCnKSupc7HqXLz35Fd%2BpCwoaROJGmJSxpRPb4X%2BOlHe83RFkrKfNN95gS8py083oVJV7rSrBT0zaSvzz55d3JTIw6XCIjEnTGpZ07CXJNalBT2p10zqTO26kLsHpcvA/nl2enaTXp8U96QZO/FfTfxRPf8X9MAmzS2JvvHKYtNsnm97Jq0yGSr2hmeiyZsM7oZTy3aIyaprY48T5OjEYyOuOEwdpdIIk3SCZd0omUuxJm89uBW7cmTFJ0lxS9Jn4mmUlLpnGSfppkpmTNKJ5zSrJwM3Kb73yngyeZ5vKGSrwFlSCtZQswSUuB3ZiyDptUtsWhN8kyyex2M/sXjKInKyIpZXYmSuw1m89ueO7bWa%2BNinvj9ZCUw2Z9KIEmyGZv0onv9JZmWS2ZHEjmaDK5krTzea0vmS7JT5uz6BCcj2VLz3Y%2BzQxR47yWjNuFGjMZLU%2BWTjKun3jw5qklWVHLVkxy12cc3nq70HHm9UxlonWZTNTnUzD%2Baw42SlNNlpTzZGUgGUT2ylFzfeS00uQ7PLm8zze/M6uSn1rlAC%2Be9c1foeybmHjxJqM9kejI7myysZ3c0OX3LCmEyh5JXdWaPKzbxyJ5vPR8VPOHEzyk5uslOeIP0mLzDJy8iaalKmnMSierEguUL3Zm7zOZvvbmYfKdmVzzerss%2BSnwvkp9Neyo8PnDJFlwSVBe06qb7IlmtzH57crsS/K7mJjgpH826ZHN6mPTSZmsgBbzzd4Zcs2JEkBc%2BPImDSlhUCg2TAtpmZyV52cs2bnOZmWzWZaCneULz3lYKy5vvCucfKrmj8a5RClPsAMPYp9beZCw4Rz3SG7SqpmouhS3OOmMLORWElhRdJ7mKz8ZA8rhdHKK6xz/548gRUAt5699T2iciiW%2BKolvTPR90JeXIvgWrzEF6U5BZlK3lAyheIMu2WDN94QzcFei/BafNH7nzjFJCsxSn3aGesaxZU%2BGb0JsUDDaFzc%2B%2BfVKklMKXFwct%2BewtCmcLt%2B38nhX4q3YBKs2gi4vse157b9z2YSiRbpKkVpyZFRsuJRMMmmtskFXIlJVeLSWjtbZQve2dksdm%2B9nZ%2BSgxYQtH7ELR%2Bpg49mUpT57CKlW4pgdfIqm1Lgxok7USjKaWBzn5bSthbjI4URzul3C3%2BXlzHlZtAFWbYZSErGXf9L2Ey7EZEqpnRKpBCSGcfEoUVrylFFs7saoq9HqL8JmCoXtgp0VHzfeJ8w5YUqMWj8TFZy1oaEtH5iDIVV8%2BGTtOoW2KDxnkl5e2OlnvK5Zny3uZ0p%2BX3Th5viv%2Bf0qBWBKQVwS0ZbzwhV79r2UKyiZYJWGzKM59grOcUJzltDlF6K1BZioWkYKS5Wig%2BfityWEr9FxgwxccuKWnLSlo/RvuMtH5WDpVdKynre1vksq6pbK06RytflcqPF/c8Kb8p8W8LAV2XQZUEuEXiqs2kq3ngf1vYyqIlcq6BTEtgXzL%2BBCCpZUkpWWby1l1s9JZspClZKheOSvZXgt94EKSVZqslSUtH6UrrVNKu1Z6OjUOr4ez7Z1c8tdUBz2VzCj5XhO5VKyvFfq/lQGv8XCrg1oq0NVm3BW88pVUa7gc%2BxjXJyYV88uFbEqVXyKVViitVWiopGAys1Gy4uZkv3k7KcFhavJcWoKXGCil5ai1ZWouXUqU%2BtamvlIJnUNrpeAYxlXUrsUNKjpD8hqS0v8luL35PK3tXyp/kjyAVg6oNXuxDUjKx1EqidZGsV7TruelLWdRAvnXTKF5Ca2RcuqRWrqUV66jefnK3WFztVGinFQ%2BO0VC9dFRqg5SaqOXGCTlxgildeuME1qU%2Bdah9fQKQ1Prwuza5Ga2qlnuqO1nKrtd6s/mDy/lIGvhQMog0jqoNqXcNbBqzZTrFeiG%2BXuF2Q1zy0Ni6xNVhoWUpqQhaagEQRtSXbrzRu6rZXmpna7Khe%2Byk9cSrPWkrjB5Ky1cYOrW3rWN96mvhxr95qauNSgh5V4I8ktr/Z/Gp%2BYJs9XCaOpPqr%2BeJoFWgahV4GoZWKug3yas2k6%2BDcpsV6qbu%2BwC5PqAuingKNNVgrTZhu%2BnIrEl685JRmoVFEabZZm3NfuvzVWbIpx6oXiWvs1lrHNFa4wVWsuVubR%2BbGzzTX280mDSF1y6CS5NX5fseNfsyWW3OcW/rAp/6ntb6qA29LBV/C4dUItk3jrUtcGp9ghsy2K9stivERblrEUjjwlc6uNdIow1zKdNyahJamoq3pqjNmamrdmrq0biGtlmw9dZqLWtbT1lvc9Z1svXdamNlvFjf1o80a8vNNfEbTX3MVjb%2BJ24qXj%2B2m30LHF36%2BbedMW0dLlt0W/1X0o23SattYKmDbtsU3pan2Kmo7Yr2L5ftFe6k0RWRIu2TK9ZmmqcYqtK04bytqK/DSos1XljatOqvdXqoPUGqj1VG2zTRtLV0bzVDG5zZb1c22r3NGvdjUNrh019TBX7GvuUou43Laxjqmpa%2BseWBbeNwWubTJJx0hy8dnilbarOA2xbJNQ6knaCrDXnsFNCXfbRlqfZZa6dJ2xnYr3JEaT5hFMyRUVs50Ir6Jumx7fpue2GaBdhGtRcRuxW6rcV5GudgSqF5EqZd7WuXReoV1XqrVvWlXVDrV2DaNew2rXQjt1018rl%2Bu8bSjsm33KTdAWpGTNoYVY6rdncv9bbqi1ibCd62qTYltHVyaPdFOr3bex92vsn2x2p9r3yC5Ptv%2BAHdTeHvlW3aud9MnnU9r52VbXt1W5PcLpI1p6yN%2BqijVnqXbGrLepq/PSDsL1g7i9N60vcYIG0w6NdGveHRr1aE/s69NfIEUjuclN68%2BQHdHQ4q/XNLsdPe3HV8oA326el/y53Qlsg1k6UtSXSnd7up2HbZ9/u%2BfYHqX2K89%2BQHVfVMoj2jSo940mPWVp314a99ie4ze9p3Ui7zN323ib9ua1S6AddmoHQ5st5Oai9LmkvcYLvXl639lezXRr213f7G%2Bf7P/TXypEAHSpqQvPiB1AONK3VoW1pUJoVmRbRN3i/tUTuH3IH3dEa9A1PswO%2B7adH7APU%2B235/sCDivA/iB2IPs7SDH08g3AsoPb649u%2Bl7XQbe2H6PtTB%2BrWLsa1sHipLWldoDv/k8HBlXWy3j1qf1CHVdAfdXWIY/3V6v9tejXmIJX0a9PRThp9ZB1UOfrXl7azQ%2BFu0NhzdDfax3QOvi3ArSdxhz3UpvMMz7wuOBhnTYaD32Gn2jh7gZB2cOQLXD6c9w0msYm86aDvhjVUnq1VH7U9ou9PWfsz2Grs9V%2BtdrRst70bLejGx/cxr60v7odAfWHekYkM16Ne0hnI1YKINB8pBAxwoy%2Bo8E0L31d8ko%2BoZ/XW72lMB/HQPv0ND6XdI%2B7beTrQOT6DtFh7A1YdwNdH8DT7Qg30ZN7c9oOgx1DcMYVWjH7t4x6gzsP53TH6DARxg8foWOn7xd5%2BlY5fuo3X6NjQK%2BXdscV1jrBDlvYQykYr0B8q9pxzI%2Bcd/0a8rj%2BR24/QMROFH6x/mpse3ox3gG3lYW1hRFqqNdLVtCBwNQ0bd3Jbx9wJlo9FzBPtGITnRxfXYZhMOGTe/RhE/L1g5InrtMyjfWie52GC11WJ2gzif8OzHAjBJ5gyEZ%2B0S6/tERp6VweiMdbeDcR4ReDvDX7HLer%2Bo4%2B/oD6f6A%2BUhzkwH25M29eTNvP3sacKN%2BbW9wp8WWAdKMCbyjkpyo98sA0O61tcW4nQCZQPKm0tGBtU20bn1anbDy%2B2E/qfhMm8jTbveDiaeGnFa7tlpxmT4YT12mD9Dp/E/MedOLHiTyxyXasfJPrHZdmx6kyCtpNUq9jz%2B4M4cfH7HHwzGRyM1kYD6XG5DcZm3nycTPADWzhRqhY8aZVPLzds2pxd3tcXQHu1dugnb8aLOGGZNpZkwyCen1%2B7NTC%2B2sz0d84QcDTTZk3i2Z76nbT%2B52sBZdpQ2mn0N8Kr4VvqtO4abTUxzdbiYHOmagjX2l06wbdPsGJz0uik9OapMF6aT/BpXfSdS3JHx%2BqRlk%2BIYD6SGtz4/Hc3kb3NB8DzNvI8wNMR0N7kdty5Q9YtTP7SP1rKttVmYW026vjD5n47UYMP/GjDSp986qZp3gnQu1h7U3Wb1MAXGzEHZsyb1Asm8mdZ2lnZBbZ1DH19cFxFZ4cQsTHkLvZ1C/aaF2OmhzwRkc6EdwvhGODkRr01rJ9OxHQd8RgMzaqSNl6mToh2iycfotnGA%2BFx2QyxZuP7mEzHFm3lxZt567Uejevi5T1Q7FGRLIW941AYkv3n%2B9ehmS38aQOvmmjE%2BpS1gY1OqXIT6lv83CYg6GngLelk3r33s4Qdg9zOzSQNOhUwWOzm%2B5VdZcxN5y/D/Zxy4OarEn7IJSxxyaSb6mTmyZRFhOSRbnNkW6TiRhk1Rf23Mnx%2BrJyK%2ByeivRnx%2BsZ%2BK2xcStB9OLNvVochxt7170rvFw3Xrxb1nm31zKoLVea71NSCrnxoq9UdlMSb5TIqxowpeaNU7KzX5uqzWe6O6nejDZ5q0BYg4gX2rBlk3t/3Q5tmolke%2BC8Ne7OTG7LVshgxhadMuWiTblkk%2BObJMEWpzeemc2tdMUbWFzEOoM4ppXPU6wz4/CM%2BPyjPZHYrAfHkwlaD6HnkrN11K/dZt7/6eLgBzK3rwZVvXTdIpjM28cgO3nCrImmUwWblNgaFTSWsfYpYhvKXar9On87Df87w2tLiNnS61Yg76WIO2/VDibz36YcsbsKnG5ZYe1UHvDBNlBTMcmsk3nLWF1y66cpvunPLnp3Pdwd8t7s/T5y3YyzaXNs2RDoZtI%2BubZObmOTfNrk7ufOt192LV10W0H1uuN9UOktm3vIeluKG/RefbDjlb42W6fratv6xrd5Va2gbOtkG4qf1vg2KzRt6s6behPm3/zHnK2652RttWIOHVh2xjedsH9sOrthde7ej2e2vD007E/ZYmvoLSbQd8myHbHNh38LnByO96dv2%2Bn/L/p%2BO4GcTvSrk7q5zm7Po3M83GLS%2B/m%2BP0FsXXhbSVoPilaD4l2xBmNoPp6PntPqCOddi3decbudrczsBx86VefNyWKrYNqq4bZqv928DOpoe01cg5I3YOE9xDmja6tO2Tec97gQRwXsc6yDuNldSNe9u2XfbaF/2xkuHO72cLodvC9TaPuEW6bxFu/aRYf0CGtrlFkK9Rb2u%2B66L4/Bi5ne3Ov3ejrFvO5dbr7XWi74toPv/asEu2Q%2BUgshyA4eOZD3rF5jvZjogM3moH7inQ5rfgPt36jndvWztpVMoP1TaDqExg/rOW3sH1t8e7bdRv230bRDiDiQ897c8iO5DlE%2BaaofYaaHa920xve3kp7prhJ2a6OfmtU3FrNN5a9w9Wu8P1r/D8i4I8hXCPdrYV/a%2BI4/ZRXx%2BMV7O3Fbr7xnP7Bd7%2B2Ld/sS2g%2B6joB1o/oHBOQHgpwS/UpeO5WG7QcrQ2Y%2BlOt3LHTu4G5tq7t2PyzZhyG5YehsD2XHmlke%2B47Hu4OvHk9gh47Yg6z3PepDoJ/LxI4hPzLS6rs6qroerLN7WK%2BJ8w8ScU3977D1J5w9ptR3T7fl%2B/QFcvtBXtrBT6NUU7EcRWJHZTmwydZhM53qnQtuviLfqfKPGnqjsu4A4z5tOM%2BfvQ5yA5TMK2296ZtQ6JY0PiXm75jkZzFrqPFn5L3d5B73dQcdGFnGli28s5aueOnOGznx4Q%2B2fEPdngTz3gc7d5kcjn8aiy8vYxO0OxrfZ2J3Meudk3bne95JwfY4deXj7Pl15zHfPtx2BHi54Kwcdvsc3U7XNx%2B/PufsyHKnAtuR5P3zuKPC7dfYu007r4tPEXGfdpyi%2BAE8uQHp5vR4raxevGcX%2BVpu16oJf5nRnxLl86DbJf2OKXjjql%2Bg5pfD3ALHjtZ4y/wfMutnNnfx%2By/w77PPe3L4vhR15c3b%2BXFBle5E%2BWWE2MVjDnNTvclesP7nHlw%2B3K64cvP6bWTxmzk82tqvvnGr0KynfCtp3DrGd461nZjNgvJ%2BNTyF1/br4/26%2Bf9%2BF3X1ad2vkXGfR16YKzcgOBLGLtM/Yuxd5XVbpjpbVJZKuFnEDut0fVM720zO%2B74b5x5G6wf0vY3dtzq4m52f4c9nnL9N570zef9DL4F4y/lqguFbjn2m059aeFcxP1lAd8V2W7xVsOq3sriO7W5Pv1uz77zi%2B6q4Tvqvlzmrmddq4fvp2n7UjpizI%2BuPguP7w7up6O4afjurXk/G15o5nch8HXGfBd4NLSvi8Zbz1pcDRzAdfXjHkDwZ9u%2BKs1G934z13bY6BPTPQTVZs9w1bhtRvtLqzlG0y9vcz22XD7jl/hy5cvvPeb7z3t1aMu9WXpa%2Bvlyc4Qv43znVW0V05dA/bK7n0rh5znpg8Ku4Pbzvhx86Q9X2UPSd9t3fYw9qasPernDy/cNdv3jXzVhR5PyUcWuVHdfNR%2BXdtch97Xc7ujxnwY8Z8HrTHqu1tJruvXXXmLtdx643cmPuPfegG23bGcd2JnQn1AyJ8/NzOTbEbxqwjZWe6X1n8b%2BT346s5KfU3T7/Dhm/U%2Be9t%2B2nfDvKJD0vjv3ennNwZ7xtnPAPRNvEyB80XB2K3VnyD485rfPPYPPD%2BD458Q%2B5OW3QjttyI7%2BdIaAXpTo6%2BU5Be5GjXudk18F%2BRvmvJ%2BlruF806i9UeYvs7kPvO4S8Z9G%2BNHDPlLcevMeqllPN1ux872ceBnFRoZ3mbgNEvZL5VoN0e9MOieob1X897V7cdXvZPTX6ey14CfteVPz7/Dq%2B56%2BafPee/N1tm7NO5uPD%2Bboz5N%2BLdb3A7FnqV5RurfQeVvdntbw5%2BydOetvyH1t6h/c9avO3Or7zzrt88Gv%2B3VTwdxC8n5QuSPMLsj/d%2BtePeG%2BSLmj3F5D70ePvSX77xnwrsWKYJsPR4EuDu6rhmOeBAHgJJE6BtVwZ0LCHJwU7nQhqld8hdCFlbytcefAQTs78eCW%2BxOE2uHob7u5fdwIwPX30Acp4y50eDIXjnGxt%2Byd5OcbRTo8CWgMl02tv%2BPy6UT%2BO/4GnIAuJaBFaMtpYBrHkJyHO7Z%2BEmBzbOBYEFYsNYmXTa5NLFMZe/XfaIDVuiV%2B%2BpfgIS4OYNgHyBeByoCIJYJCAgDAtVw1gHCFCxcDAsxAIBGf9NnG4mQGg4PFL175H9j%2B4QjwFED37RA4Rh/2AUf%2BP945T/Z/x/3iuN16ImFNMFmfVE4X%2BCr%2BD/k/7AA%2BUMJPlrSpMA%2BHHn/SNA7/6/w/9gFfJ6%2BBJb/w38t/cfzRB4QXf338f/B/2n8T/L/AK56Uc/1MIr/EQBv8/gIAN/9ygAiFSZtcFWA7h0A6AI9YnfaeiADN/fIG391/SEFh5t/CALX8J/I/1gDP8eALyxEAy/3fRr/MNnwDgWcoBNIMCOuhwJ2wROkqRXGfJDwC9/WgN/870NWh2pCA9vxX8xA8f1IDyA%2BTiEAhAGgPv96AhgKhwmA6HGnJAcC/3tJkA1AM4DH/GknNJTSeTBtoBA4BktosA4QNwCQAYwI9xbAuuHsDjAvmn/8xkIgPR4SAkAPX80QFwDmA4QNQKgCNAzQL%2BxtAz3Bf99Aj7EMCOA%2BQJCCTA8/z/x%2BkUHC/94gugMSDdA73DnwUgrLDSDIAjII8CvAwAJcAAAaSeAXSbAEhBIIOECSBggwoLCCZ/CIP2pM0FPDlRGgjoNGwIg01iKAvfJGgQpOgzQIiCBAQ8i7IFaRiiVpWKGGnYoR0bXHUgI0UQIKDf/XsEww68B6kYDxuA%2BgqQbSKILnIjofIPEDoAk9EGCIcYYNFoNyJIN9wDg9QIqJHkE4OAIIgn%2BHuDngpoM2CXg94I2D16HoPNYg/A2gGCPgh4LeCL6cYNko86LmgLppghkl/I40NHHDQegRYMODgWAghsoGAs4K2D7KOIKWDoAq5ABDYAs4K8pMQxENuDcQzoMeCJkEkIpC5/VrkpCaQ7oKNAzWPoN/pZYGkNOCgQsyiyoQQrqnzpFKIOhmDoQuYLhCEQm4ORDsyE/zRDHqDEJGhsAkQIcD0giQJxCWQnjDeCCQqULsDvkWUKxCkQ%2B9EVDwg%2Bf3JCdQykPFCDQkkLpDdgXoN%2BDjKQ2mNCzcNkOtpbGeWiIwVqMEOVpIQrdFjQBQhYI1CiQkUPaDXg1Th4DU0VwksChAQQOcCcA9UMcDMA1UJcCIwuUIf9KIC2ApgqQ77AJC1KZRGtDSQvUJogMw3EKNCcwl4NNCGQ/4PzDUQt4I7pc6ZinBCeQ5SlmDYQz0LcCK4dYNLD/QtAnMCLkZABHBBA6UNcC4wqf3YwVEd8kr8wwGv2sxxwDxnFg8AKWDFBiCdbHKBrGYUFFAQmcLGiYAwdllCBlmZKHQAmmacNiAMgEgHsAAoTwFsAMWDIANhtKfoGnBVwoyAPRnGFgCrQdgKYEWYrmUKFiAcEeMDWYBcX/39AMAGZkyYuadhlERcgePEPD%2BAXlltZLAX1DyAtoHYHOACmdSEShcgHcNEZPw%2BMIpJxKCuGxYigcWFtAQIp9CWBUAYIBbgwIovzlBSGAMgwYfSd4GlhgybokKZiAewCxYjmFFjqBS4QcDFYYgTyDmAwATgFCB/SN8CfRV4Zvz6Z8EcIG4ilAC5jyB9GZgAoigKBnCkYQaVoBxgEEXYGUYRGeoFWYHAHJg4YdmLSLDwqSASLegtwnZioZRGIJCMiiSYIErAY4EAAMiWIWgBbgvATpFUZPIeYIwRHAbDgvRXI9yNUAhmYgg%2BYBUBnA7QqGIOFwYIUGZCNhpwwkg0gTiECIqxySMUAkixQFyK5IlmLIBlBQyWIBHC9uYMhkiYqCYHQQMGfOCIAAycIFijV4ZQBbg5geFEiRYwHbiWZMAbiN4i8AciLlBsmfagvDYwcWFVAxwsWElgzQjxFIisgciMfAigXRlIZJAGWFsjV4CbAEZ2GVZgKZ7QckF6YRYccL6j/kEnGO4Q8UKkoJWgZJnYoVo3qMnD%2BokoEMZgkW0Cmi3obyFIYWwURlmRaYb9C2A3wZIAkZZYAMisYrIEhGnCRQO0EtBFII0D6g%2BYcPGGAPo6cOPgaycYHXggkNqNWR3mHZAxxG/TkMmDuaaYLlQLI%2BhHVY2/Zfw5QsGJ4H8x90KDAwCPfM3xLC4uVwDujCYUwKup7YRhBJitAsDnJj5kbtAVDaYuALeCWY%2B4KYCWgqiAwj2YvELA57YQfBQiuAq2BtgMImmN5i8w3mLCCIg9ujVDo4S2CFjiQqWP%2BI2Y5WMaDCw80PR5%2BgjKlfoF6a0LODRQ0MP%2BBFY44LViWuFMKgYw2RWPTCzYpUPXong22KGDVYx2LFCz/ekM1iBJJGhtZ10ScM3QL0MdHApLYHyMzDh2GVkxA5WNEFD8kBLvx78%2B/MCAH8c4GTjt8E/B3xtjBgyWJdiQCZoO0pWg%2Bgl1oeYqWIiCvuEUgNpvgkWKfAsSFIispKY8GmSA68DAnf9iSf9F1D7YpMMzjXY0OMj9w4t32ID0gxQJwhU/OP3t8k/E4LJDsw9uNn8M4ieLpj16LmPHxwifSlcxnMeHANCi4rWJLjLQsuNLgRQRDGkAq4y4kixMMSvGria8ZIHNBMyYXlZDW4u4Onis4t4LDj0IHuO8C%2B43wMeB/AwIMHjk4jPwd9pYrMNvi/Q/en/ifsbOK1oOcLPF9Db4teM9iN4ovFbgzQ81m3jQgTokWpziJvGQDmwh1DbigEz4O%2BwH4iOKjjLWHwLIDQA%2BEE/j0/TP3Vi/47BJwSqaahOUJxuOeLYR3sbBKgTtxFghgxS492IQSy4SmPyRUE1IkiwYAjBPpRxYuhKvjcEruMfjI43uIKD%2B4igKoCTApOPISf4p2Nbjx4sRN%2Bwp4jROTCgSHOLAT6CCBOnjWEpAXYShcThPgTy43hJdp%2BE6uMES%2BY6%2BO0SW4iRIElm/AhIFwiEpQJcAVAshOHihE%2BxMwT1ExxPoTqQoJLXg6Q1NDdCq0D0LcisE/%2BLODXI/2mJwaYpROHiH0bUNtix4tONCSdE4amyTV8cbjwSn4koPKDHgSoOqCcIUIhbY0/XxILDnYvJNXgNYn4K1jRSG1nOBUAXbgmxd8D%2BCyT3gpgMdR0k%2BpLCS6kwZNP8h8B2JGShksZP1CJk/JKHxvgmEhpRLAAgGTo74KMMGwqxR9AmQpkS0AZiiYVsIwIl/TGFBBPAaenRBMQbEFxB8QQkAXB7YZVFlgIGAQGjIsgaAgEBUgDFlEZJmM/D6ZmAJuE2BPaD1mxii4VKiR54gNMH/JygZAA8D7YTADCRJSKwAeSvIeIF25EImZk7oaIKpA9ZmEPRPnjqEfgkQT1gH2CFQMaCABtZBYHIB5AIU/8LFRJUOKBtZqUsaAIBheBFIEp8EVFJMQUU8pClxKkSvBmgq6LLBeSH0CwkboQKdnBxTs4aeDaTduN/xhSSUqlMPRZQWlLlTJUJlJFoYUuFJpIBUgMJrhn/YgDRwQ8blINgG8A0FLpzg%2BoANBGgDlInRkcbGElTgEUwBx4bWO5PlTUgRVKUg1ke5LGhUgFVJvonaaVLCQkeOKC9S4GFwFtTggUwGp5HU%2BIGdTXUxICdSiIINOjpTUiYCaAXuXlNugvUxoBe5q4l1LOBroS2AzT/AKXBzTzWaiHzTlARoA7DG6TSncBoyUhmGAUKBFKdROAbTCmAuADQE4BSAcIC4BVADtNQAuAGJDiRHkJgFYA6YMNloAO0zAk4Ae05P3sAQADzlbTOAcyA7Su0qdN7SuADtNPh12SdOnTSAOAFgAkACMCjAyACgFlS%2BgI9MuwBgYAGPZ12SYGxw3ESgHOhXgSgG0x/AbtI7SfEFnFAgQkaQDfTSAfADCZao/BF/SqAHRmIAf0zgD%2BBGgctPHTSAGYBGAYgOgFIBLGQUFAggUEFAfTzoVQGfTYM9SGAAEM%2BgGQzUMmYkwIbfJ9OoAAARTvgwMndnug/2CyBA4iOQ9jmgRoeiH0Bn2SgFhilQcDObSW03DPwyFyUgCkB/kU%2BE4BtAUCHNBtAbeGogOw2JFigdAIKGwiN0lgDYAOAAAgXT20ztN/S%2B0zgHeBaIfCG0AyOAxEvS%2BASDK0AIAXAApxxUfpXEB2SI9KszEkCdLfSZ0udPXYF0pdM0zV0wTPXTGAEAC3SnMltK4BlAZdK0zvM7dJ4zSAanDcgQAcyCAA%3D%3D%3D).

## Visitation without an instance

Besides iteration over an *instance* of a registered struct, **visit_struct** also
supports visiting the *definition* of the struct. In this case, instead of passing
you the field name and the field value within some instance, it passes you the
field name and the *pointer to member* corresponding to that field.

Suppose that you are serializing many structs in your program as json. You might also want to be able to emit the json schema associated
to each struct that your program is expecting, especially to produce good diagnostics if loading the data fails. When you visit without
an instance, you can get all the type information for the struct, but you don't have to actually instantiate it, which might be complicated or expensive.


### `visit_pointers`

The function call

```c++
visit_struct::visit_pointers<my_type>(v);
```

is similar to

```c++
v("a", &my_type::a);
v("b", &my_type::b);
v("c", &my_type::c);
```

These may be especially useful when you have a C++14 compiler which has proper `constexpr` support.
In that case, these visitations are `constexpr` also, so you can use this
for some nifty metaprogramming purposes. (For an example, check out [test_fully_visitable.cpp](./test_fully_visitable.cpp).)

There are two alternate versions of this visitation.

### `visit_types`

This function call

```c++
visit_struct::visit_types<my_type>(v);
```

is similar to

```c++
v("a", visit_struct::type_c<a>());
v("b", visit_struct::type_c<b>());
v("c", visit_struct::type_c<c>());
```

Here, `type_c` is just a tag, so that your visitor can take appropriate action using tag dispatch.
This syntax is a little simpler than the pointer to member syntax.

### `visit_accessors`

In the third version, you get passed an "accessor", that is, a function object that implements the function computed by
the pointer-to-member.

This call

```c++
visit_struct::visit_accessors<my_type>(v);
```

is roughly similar to

```c++
v("a", [](auto s) { return s.a; });
v("b", [](auto s) { return s.b; });
v("c", [](auto s) { return s.c; });
```

Accessors are convenient because they can be used easily with other standard algorithms that require function objects,
they avoid the syntax of member pointers, and because they are well-supported by hana and fusion.

Much thanks to Jarod42 for this patch and subsequent suggestions.




**Note:** The compatibility headers for `boost::fusion` and `boost::hana` don't
currently support `visit_pointers`. They only support `visit_types`, and `visit_accessors`.

To my knowledge, there is no way to get the pointers-to-members from `boost::fusion` or `boost::hana`.
That is, there is no publicly exposed interface to get them.


If you really want or need to be able to get the pointers to members, that's a pretty good reason to use `visit_struct` honestly.
If you think you need the fusion or hana compatibility, then you should probably avoid anything to do with member pointers here, and stick to accessors instead.

## Tuple Methods, Indexed Access

`for_each` is quite powerful, and by crafting special visitors, there is a lot that you can do with it.

However, one thing that you cannot easily do is implement `std::tuple`
methods, like `std::get<i>` to get the `i`'th member of the struct.
Most if not all libraries that support struct-field reflection support this in some way.
So, we decided that we should support this also.

We didn't change our implementation of `for_each`, which works well on all targets.
But we have added new functions which allow indexed access to structures, and to the metadata.

### `get`

```c++
visit_struct::get<i>(s);
```

Gets (a reference to) the `i`'th visitable member of the struct `s`. Index is 0-based. Analogous to `std::get`.

### `get_name`

```c++
visit_struct::get_name<i, S>();
visit_struct::get_name<i>(s);
```

Gets a string constant representing the name of the `i`'th member of the struct type `S`. The struct type may be passed as a second template parameter.
If an instance is available, it may be passed as an argument, and the struct type will be deduced (the argument will not be accessed).

### `get_pointer`

```c++
visit_struct::get_pointer<i, S>();
visit_struct::get_pointer<i>(s);
```

Gets the pointer-to-member for the `i`'th visitable element of the struct type `S`.

### `get_accessor`

```c++
visit_struct::get_accessor<i, S>();
visit_struct::get_accessor<i>(s);
```

Gets the accessor corresponding to the `i`'th visitable element of the struct type `S`.

### `type_at`

```c++
visit_struct::type_at<i, S>
```

This alias template gives the declared type of the `i`'th member of `S`.

### `field_count`

```c++
visit_struct::field_count<S>();
visit_struct::field_count(s);
```

Gets a `size_t` which tells how many visitable fields there are.

## Other functions

### `get_name` (no index)

```c++
visit_struct::get_name<S>();
visit_struct::get_name(s);
```

Gets a string constant representing the name of the structure. The string here is the token that you passed to the `visit_struct` macro in order to register the structure.

This could be useful for error messages. E.g. "Failed to match json input with struct of type 'foo', layout: ..."

There are other ways to get a name for the type, such as `typeid`, but it has implementation-defined behavior and sometimes gives a mangled name. However, the `visit_struct` name might not
always be acceptable either -- it might contain namespaces, or not, depending on if you use standard or intrusive syntax, for instance.

Since the programmer is already taking the trouble of passing this name into a macro to register the struct, we think we might as well give programmatic access to that string if they want it.

Note that there is no equivalent feature in `fusion` or `hana` to the best of my knowledge, so there's no support for this in the compatibility headers.

### `apply_visitor`

```c++
visit_struct::apply_visitor(v, s);
visit_struct::apply_visitor(v, s1, s2);
```

This is an alternate syntax for `for_each`. The only difference is that the visitor comes first rather than last.

Historically, `apply_visitor` is a much older part of `visit_struct` than `for_each`. Its syntax is similar to `boost::apply_visitor` from the `boost::variant` library.
For a long time, `apply_visitor` was the only function in the library.

However, experience has shown that `for_each` is little nicer syntax than `apply_visitor`. It reads more like a for loop -- the bounds of the loop come first, which are the structure, then the body of the loop, which is repeated.

Additionally, in C++14 one may often use generic lambdas. Then the code is a little more readable if the lambda comes last, since it may span several lines of code.

(I won't say I wasn't influenced by ldionne's opinion. He makes this same point in the `boost::hana` docs [here](http://www.boost.org/doc/libs/1_63_0/libs/hana/doc/html/index.html#tutorial-rationales-parameters).)

So, nowadays I prefer and recommend `for_each`.  The original `apply_visitor` syntax isn't going to be deprecated or broken though.

### `traits::is_visitable`

```c++
visit_struct::traits::is_visitable<S>::value
```

This type trait can be used to check if a structure is visitable. The above expression should resolve to boolean true or false. I consider it part of the forward-facing interface, you can use it in SFINAE to easily select types that `visit_struct` knows how to use.

## Limits

When using `VISITABLE_STRUCT`, the maximum number of members which can be registered
is `visit_struct::max_visitable_members`, which is by default 69.

When using the intrusive syntax, the maximum number of members is `visit_struct::max_visitable_members_intrusive`,
which is by default 100.

These limits can both be increased, see the source comments and also [IMPLEMENTATION_NOTES.md](/IMPLEMENTATION_NOTES.md).

## Compiler Support

**visit_struct** targets C++11 -- you need to have r-value references at least, and for the intrusive syntax, you need
variadic templates also.

**visit_struct** is known to work with versions of gcc `>= 4.8.2` and versions of clang `>= 3.5`.

The appveyor build tests against MSVC 2013, 2015, 2017.

MSVC 2015 is believed to be fully supported.

For MSVC 2013, the basic syntax is supported, the intrusive syntax doesn't work there and now isn't tested.
Again, patches welcome.

Much thanks again to Jarod42 for significant patches related to MSVC support.

## Constexpr Correctness

`visit_struct` attempts to target three different levels of `constexpr` support.

- No support
- C++11 support
- C++14 extended support

This is controlled by two macros `VISIT_STRUCT_CONSTEXPR` and `VISIT_STRUCT_CXX14_CONSTEXPR`. We use these tokens where we would use the `constexpr` keyword.

In the `visit_struct.hpp` header, these macros are defined to either `constexpr` or nothing.

We attempt to guess the appropriate setting by inspecting the preprocessor symbols `__cplusplus` and `_MSC_VER`.

If it doesn't work on your compiler, please open a github issue, especially if you know how to fix it :)

In the meantime, if you don't want to tweak the headers for your project, you can override the behavior by defining these macros yourself, before including `visit_struct.hpp`.
If the header sees that you have defined them it won't touch them and will defer to your settings. In most cases this should not be necessary.

On gcc and clang, we assume at least C++11 constexpr support. If you enabled a later standard using `-std=...`, we turn on the full `constexpr`.

On MSVC currently the settings are:

- VS2013: no support
- VS2015: C++11 support
- VS2017: C++14 extended support

## Licensing and Distribution

**visit_struct** is available under the boost software license.

## See also

* [map-macro](https://github.com/swansontec/map-macro) from swansontec
* [boost-hana](http://www.boost.org/doc/libs/1_61_0/libs/hana/doc/html/index.html) from ldionne
* [pod flat reflection](https://github.com/apolukhin/magic_get) from apolukhin
* [self-aware structs](http://duriansoftware.com/joe/Self-aware-struct-like-types-in-C++11.html) from jckarter
