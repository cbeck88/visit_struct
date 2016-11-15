Implementation Notes
====================

There's not much code so this will be brief.

Goal here is to explain why things are done a certain way when this isn't appropriate
for the main docu, or code comments.

## Trait

The first thing that isn't clear from docu is that there is in fact a type trait,
`visit_struct::traits::visitable`. `apply_visitor` is defined in terms of the trait,
and the macros / registration mechanisms work by specializing the trait.

Conceivably the trait doesn't need to exist, but if it doesn't then the intrusive
syntax and the regular syntax can't play together nicely, as the intrusive one
sort of needs to have a trait to work. I think in general a trait tends to give
better error messages than just one massive overloaded function.

It also gives the user a way to statically assert that a structure is visitable. Even though
that's not documented, most users savvy enough to want something like that
probably are going to peek at the source code anyways at some point, it really is not
that much.

So, I think using a trait is probably worth it.

## `constexpr` support

`visit_struct` assumes that you have `constexpr` -- it targets C++11 and this is a C++11
feature. In C++14 however, `constexpr` became more powerful and void functions like the `apply_visitor` function
itself can be made `constexpr`.

Currently we indicate `C++14-constexpr` things with a macro, and this is enabled by a preprocessor test:

```
# if (defined _MSC_VER) || (!defined __cplusplus) || (__cplusplus == 201103L)
#   define VISIT_STRUCT_CONSTEXPR
# else
#   define VISIT_STRUCT_CONSTEXPR constexpr
# endif
```

It would be better if we could actually test for the feature using SFINAE somehow I guess, but I'm not sure
how to do that. There might be a good way to improve this test -- using `__cplusplus` value to test version
is not actually that wise, and this may cause problems for certain compilers.

I suspect that users who have issues here will be able to fix it pretty easily on their own.


## Intrusive Syntax Details and Limitations

When using the intrusive syntax, the macro `VISITABLE` declares a member and declares
a secret typedef containing the "metadata", i.e. the name and the member pointer. Besides
this, it uses a trick to add this type to a list associated to the class. The list is updated
basically by using overload resolution, see code comments for details. The `END_VISITABLES`
macro finalizes the list, and provides a final typedef which the `apply_visitor` impl can hook
into to get the list of members. The rest is pretty standard, we use pack expansion to perform
visitation, the hard part is just getting the list.

One unfortunate part of this is the when we make a list of length `n` this way, it requires
template recursion depth `n` from the compiler. This means that there is a practical limit of
say 100 or 200 in these lists. Currently we code it at 100, because boost `bjam` likes to
impose this limit on template depth by default.

If you run up against this limit, you can simply increase the number in our header, and / or try to pass appropriate flags
to your compiler to increase the template limits for your implementation.

If that's not possible, another way would be to change our implementation. The basic issue is the `rank` template, when
we instantiate `rank<n>` it forces depth `n` template recursion. What we really want is, instead of
indexing the list by a single "number" (rank), index it by a pair or a tuple of "numbers". However,
overload resolution won't always be unambiguous if we make the function take multiple parameters.

I think there might be a solution in which we basically do have a function with multiple parameters, but it is
`curried`, so the main function takes a rank, and returns a type containing a function which also takes
a rank, which returns the list, or something like this.

I'm not sure, I didn't attempt to implement it. And I'm sure that it will make template error messages much more
complicated in cases when a visitor cannot be applied for whatever reason.

Hopefully no one desperately needs to support more than a hundred things with this syntax, but if you do, that's
what I would attempt.

Another thing you can do in such a situation is try to break up your structs with hundreds of things into structs containing structs with
many fewer things, and visit them in a hierarchical fashion. That might be more organized anyways.

## Basic Syntax Details

When using the basic syntax, the macro `VISITABLE_STRUCT` specializes the main trait for a given structure, and generates the
various `apply_visitor` implementations. When we do this, we get from the user the list of members of the structure, but in the C preprocessor
it's not that easy to iterate over an arbitrarily sized list.

Originally, we used a `map-macro` developed by swansontec, see the README. However, this macro doesn't actually work with the MSVC preprocessor --
it seems there are some discrepancies with regards to variadic macros between MSVC and clang / gcc. At one point I took a stab at fixing it,
but the issue is rather subtle and I don't have a good test environment, as I don't run windows.

The swansontec macro is relatively succinct and looks pretty clean, however, even besides the portability issues there are some issues that it
generates rather ugly error messages when it doesn't work.

For instance, a pretty common mistake is a typo like this:

```
  struct my_type {
    int a;
    int b;
    int c;
  };
  
  VISITABLE_STRUCT(my_type, a, b, d);
```

Since `d` is not a member, we're going to get an error, and because the error is in the code generated by the macro, a compiler like gcc or clang
is going to spit out the whole preprocessor stack associated to it. It turns out that even though the swansontec macro looks quite succinct, the
error messages it makes are rather verbose because it makes the compiler do a lot of work.

For instance, the stated recursion limit of the "reference" implementation of the map macro is 365 -- that is, you can use it with lists of as many
as 365 parameters. One of the strong points of that implementation is that it is easy to extend it. If you add another line here and define `EVAL5` continuing
the pattern, it gets a new limit of more than a thousand items.

However, one of the drawbacks is that actually, every time the map macro is used it causes macro expansion to take place 365 times, even when the list is very short, like one or two items.

The consequence of this is that even that tiny program above, when it is compiled, generates a two-thousand line long error message with `gcc 5.4.1`.

Most lines look like this:

```
foo.cpp:9:1: note: in expansion of macro ‘VISITABLE_STRUCT’
 VISITABLE_STRUCT(my_type, a, b, d);
 ^
foo.cpp: In static member function ‘static void visit_struct::traits::visitable<my_type, void>::apply(V&&)’:
include/visit_struct/visit_struct.hpp:111:43: error: ‘d’ is not a member of ‘self_type {aka my_type}’
   std::forward<V>(visitor)(#MEMBER_NAME, &self_type::MEMBER_NAME);
                                           ^
include/visit_struct/visit_struct.hpp:81:40: note: in definition of macro ‘VISIT_STRUCT_PP_MAP_EVAL0’
 #define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
                                        ^
include/visit_struct/visit_struct.hpp:82:66: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL0’
 #define VISIT_STRUCT_PP_MAP_EVAL1(...) VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(__VA_ARGS__)))
                                                                  ^
include/visit_struct/visit_struct.hpp:82:92: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL0’
 #define VISIT_STRUCT_PP_MAP_EVAL1(...) VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(__VA_ARGS__)))
                                                                                            ^
include/visit_struct/visit_struct.hpp:83:40: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL1’
 #define VISIT_STRUCT_PP_MAP_EVAL2(...) VISIT_STRUCT_PP_MAP_EVAL1(VISIT_STRUCT_PP_MAP_EVAL1(VISIT_STRUCT_PP_MAP_EVAL1(__VA_ARGS__)))
                                        ^
include/visit_struct/visit_struct.hpp:82:40: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL0’
 #define VISIT_STRUCT_PP_MAP_EVAL1(...) VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(__VA_ARGS__)))
                                        ^
include/visit_struct/visit_struct.hpp:82:66: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL0’
 #define VISIT_STRUCT_PP_MAP_EVAL1(...) VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(__VA_ARGS__)))
                                                                  ^
include/visit_struct/visit_struct.hpp:82:92: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL0’
 #define VISIT_STRUCT_PP_MAP_EVAL1(...) VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(__VA_ARGS__)))
                                                                                            ^
include/visit_struct/visit_struct.hpp:83:66: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL1’
 #define VISIT_STRUCT_PP_MAP_EVAL2(...) VISIT_STRUCT_PP_MAP_EVAL1(VISIT_STRUCT_PP_MAP_EVAL1(VISIT_STRUCT_PP_MAP_EVAL1(__VA_ARGS__)))
                                                                  ^
include/visit_struct/visit_struct.hpp:82:40: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL0’
 #define VISIT_STRUCT_PP_MAP_EVAL1(...) VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(__VA_ARGS__)))
                                        ^
include/visit_struct/visit_struct.hpp:82:66: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL0’
 #define VISIT_STRUCT_PP_MAP_EVAL1(...) VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(__VA_ARGS__)))
                                                                  ^
include/visit_struct/visit_struct.hpp:82:92: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL0’
 #define VISIT_STRUCT_PP_MAP_EVAL1(...) VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(__VA_ARGS__)))
                                                                                            ^
include/visit_struct/visit_struct.hpp:83:92: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL1’
 #define VISIT_STRUCT_PP_MAP_EVAL2(...) VISIT_STRUCT_PP_MAP_EVAL1(VISIT_STRUCT_PP_MAP_EVAL1(VISIT_STRUCT_PP_MAP_EVAL1(__VA_ARGS__)))
                                                                                            ^
include/visit_struct/visit_struct.hpp:84:40: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL2’
 #define VISIT_STRUCT_PP_MAP_EVAL3(...) VISIT_STRUCT_PP_MAP_EVAL2(VISIT_STRUCT_PP_MAP_EVAL2(VISIT_STRUCT_PP_MAP_EVAL2(__VA_ARGS__)))
                                        ^
include/visit_struct/visit_struct.hpp:82:40: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL0’
 #define VISIT_STRUCT_PP_MAP_EVAL1(...) VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(__VA_ARGS__)))
                                        ^
include/visit_struct/visit_struct.hpp:82:66: note: in expansion of macro ‘VISIT_STRUCT_PP_MAP_EVAL0’
 #define VISIT_STRUCT_PP_MAP_EVAL1(...) VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(VISIT_STRUCT_PP_MAP_EVAL0(__VA_ARGS__)))
                                                                  ^

...

```

`clang-3.8.0` gives a somewhat better error message:

```
$ clang++ -std=c++11 -Iinclude/ foo.cpp
foo.cpp:9:33: error: no member named 'd' in 'my_type'
VISITABLE_STRUCT(my_type, a, b, d);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~
include/visit_struct/visit_struct.hpp:124:53: note: expanded from macro 'VISITABLE_STRUCT'
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER, __VA_ARGS__)                                   \
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~
include/visit_struct/visit_struct.hpp:98:86: note: expanded from macro 'VISIT_STRUCT_PP_MAP'
#define VISIT_STRUCT_PP_MAP(f, ...) VISIT_STRUCT_PP_MAP_EVAL(VISIT_STRUCT_PP_MAP1(f, __VA_ARGS__, (), 0))
                                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~
include/visit_struct/visit_struct.hpp:97:114: note: expanded from macro 'VISIT_STRUCT_PP_MAP1'
#define VISIT_STRUCT_PP_MAP1(f, x, peek, ...) f(x) VISIT_STRUCT_PP_MAP_NEXT(peek, VISIT_STRUCT_PP_MAP0)(f, peek, __VA_ARGS__)
                                                                                                                 ^
note: (skipping 364 expansions in backtrace; use -fmacro-backtrace-limit=0 to see all)
include/visit_struct/visit_struct.hpp:81:40: note: expanded from macro 'VISIT_STRUCT_PP_MAP_EVAL0'
#define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
                                       ^~~~~~~~~~~
include/visit_struct/visit_struct.hpp:81:40: note: expanded from macro 'VISIT_STRUCT_PP_MAP_EVAL0'
#define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
                                       ^~~~~~~~~~~
include/visit_struct/visit_struct.hpp:81:40: note: expanded from macro 'VISIT_STRUCT_PP_MAP_EVAL0'
#define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
                                       ^~~~~~~~~~~
foo.cpp:9:33: error: no member named 'd' in 'my_type'
VISITABLE_STRUCT(my_type, a, b, d);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~
include/visit_struct/visit_struct.hpp:130:53: note: expanded from macro 'VISITABLE_STRUCT'
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER, __VA_ARGS__)                                   \
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~
include/visit_struct/visit_struct.hpp:98:86: note: expanded from macro 'VISIT_STRUCT_PP_MAP'
#define VISIT_STRUCT_PP_MAP(f, ...) VISIT_STRUCT_PP_MAP_EVAL(VISIT_STRUCT_PP_MAP1(f, __VA_ARGS__, (), 0))
                                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~
include/visit_struct/visit_struct.hpp:97:114: note: expanded from macro 'VISIT_STRUCT_PP_MAP1'
#define VISIT_STRUCT_PP_MAP1(f, x, peek, ...) f(x) VISIT_STRUCT_PP_MAP_NEXT(peek, VISIT_STRUCT_PP_MAP0)(f, peek, __VA_ARGS__)
                                                                                                                 ^
note: (skipping 364 expansions in backtrace; use -fmacro-backtrace-limit=0 to see all)
include/visit_struct/visit_struct.hpp:81:40: note: expanded from macro 'VISIT_STRUCT_PP_MAP_EVAL0'
#define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
                                       ^~~~~~~~~~~
include/visit_struct/visit_struct.hpp:81:40: note: expanded from macro 'VISIT_STRUCT_PP_MAP_EVAL0'
#define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
                                       ^~~~~~~~~~~
include/visit_struct/visit_struct.hpp:81:40: note: expanded from macro 'VISIT_STRUCT_PP_MAP_EVAL0'
#define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
                                       ^~~~~~~~~~~
foo.cpp:9:33: error: no member named 'd' in 'my_type'
VISITABLE_STRUCT(my_type, a, b, d);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~
include/visit_struct/visit_struct.hpp:136:58: note: expanded from macro 'VISITABLE_STRUCT'
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER_MOVE, __VA_ARGS__)                              \
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~
include/visit_struct/visit_struct.hpp:98:86: note: expanded from macro 'VISIT_STRUCT_PP_MAP'
#define VISIT_STRUCT_PP_MAP(f, ...) VISIT_STRUCT_PP_MAP_EVAL(VISIT_STRUCT_PP_MAP1(f, __VA_ARGS__, (), 0))
                                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~
include/visit_struct/visit_struct.hpp:97:114: note: expanded from macro 'VISIT_STRUCT_PP_MAP1'
#define VISIT_STRUCT_PP_MAP1(f, x, peek, ...) f(x) VISIT_STRUCT_PP_MAP_NEXT(peek, VISIT_STRUCT_PP_MAP0)(f, peek, __VA_ARGS__)
                                                                                                                 ^
note: (skipping 364 expansions in backtrace; use -fmacro-backtrace-limit=0 to see all)
include/visit_struct/visit_struct.hpp:81:40: note: expanded from macro 'VISIT_STRUCT_PP_MAP_EVAL0'
#define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
                                       ^~~~~~~~~~~
include/visit_struct/visit_struct.hpp:81:40: note: expanded from macro 'VISIT_STRUCT_PP_MAP_EVAL0'
#define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
                                       ^~~~~~~~~~~
include/visit_struct/visit_struct.hpp:81:40: note: expanded from macro 'VISIT_STRUCT_PP_MAP_EVAL0'
#define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
                                       ^~~~~~~~~~~
foo.cpp:9:33: error: no member named 'd' in 'my_type'
VISITABLE_STRUCT(my_type, a, b, d);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~
include/visit_struct/visit_struct.hpp:142:58: note: expanded from macro 'VISITABLE_STRUCT'
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER_TYPE, __VA_ARGS__)                              \
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~
include/visit_struct/visit_struct.hpp:98:86: note: expanded from macro 'VISIT_STRUCT_PP_MAP'
#define VISIT_STRUCT_PP_MAP(f, ...) VISIT_STRUCT_PP_MAP_EVAL(VISIT_STRUCT_PP_MAP1(f, __VA_ARGS__, (), 0))
                                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~
include/visit_struct/visit_struct.hpp:97:114: note: expanded from macro 'VISIT_STRUCT_PP_MAP1'
#define VISIT_STRUCT_PP_MAP1(f, x, peek, ...) f(x) VISIT_STRUCT_PP_MAP_NEXT(peek, VISIT_STRUCT_PP_MAP0)(f, peek, __VA_ARGS__)
                                                                                                                 ^
note: (skipping 364 expansions in backtrace; use -fmacro-backtrace-limit=0 to see all)
include/visit_struct/visit_struct.hpp:81:40: note: expanded from macro 'VISIT_STRUCT_PP_MAP_EVAL0'
#define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
                                       ^~~~~~~~~~~
include/visit_struct/visit_struct.hpp:81:40: note: expanded from macro 'VISIT_STRUCT_PP_MAP_EVAL0'
#define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
                                       ^~~~~~~~~~~
include/visit_struct/visit_struct.hpp:81:40: note: expanded from macro 'VISIT_STRUCT_PP_MAP_EVAL0'
#define VISIT_STRUCT_PP_MAP_EVAL0(...) __VA_ARGS__
                                       ^~~~~~~~~~~
4 errors generated.
```

If this is the only issue, I think I could live with error messages like this from clang, at least it even gets the right problem
in the first line of the message. But the upshot is that the swansontec macro might be a little too clever and make the preprocessor do a little more work than is ideal
for `visit_struct` -- clearly it relies on some standard assumptions that MSVC unfortunately breaks. Since I don't actually use MSVC though, this issue wasn't really a priority
for me and I let it sit for a while. I did report the bug on github to swansontec, and mentioned that I tried to fix it at one point.

Eventually, Jarod42 made a PR in which he proposed to use a different strategy. Instead of the recursion trick, we do something more brute-force:
first, restrict attention to lists of size at most 69. Then, make a (standard) macro that figures out how many arguments, at most 69, were passed to a given macro.
Then, define 69 macros which each apply a given macro to their remaining arguments.

This kind of thing is actually pretty standard macro trickery, and it's known to work on gcc, clang, msvc, since a long time, I've seen it in several other projects.
It's unfortunately considerably more verbose than the swansontec idea, though not complicated.

However, and this is the other main benefit, the error messages are not more verbose. For instance, with Jarod42's patch, the gcc error message now looks more like that clang error
message, and there are not many levels of macro expansions.

To me, getting error messages that are ten to a hundred times shorter is far more important than supporting more than 69 entries, most people will never need close to so many.

As a compromise though, I do place some value on it being extensible, that is, a programmer who needs more than 69 should ideally be able to get that without a lot of trouble, like with the swansontec macro.

What I decided to do instead is, go with the "dumbest" and most transparent map macro that could work, and which will give the best error messages possible, somewhat similar to Jarod42's patch. But, I decided to throw together a quick python program that generates this code, so that if someone needs it, they can just rerun the script with a different number. That python script lives at `generate_pp_map.py` now in the repository.

Additionally, we now define a constant that states the current argument limit of `visit_struct` so that people can make static assertions about it if they have problems with this.
