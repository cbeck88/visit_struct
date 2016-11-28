#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright © 2016 Chris Beck
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

# Description:
# Takes, optionally, a number, the default is 69. Generates a C preprocessor macro
# `VISIT_STRUCT_PP_MAP` appropriate for use in visit_struct.
#
# The output of this program should be captured and copy-pasted into into the
# `visit_struct.hpp` file if you wish to change the built-in limit on the number
# of visited members.

import sys
import argparse

argparser = argparse.ArgumentParser(description='Generate a PP_MAP macro for visit_struct.')
argparser.add_argument('--limit', type=int, help='Numerical limit on number of visited members for the resulting macro. Default is 69.')
args = argparser.parse_args()

if args.limit is None:
  args.limit = 69

def write(s):
  sys.stdout.write(str(s))

def writeln(s = ""):
  sys.stdout.write(str(s))
  sys.stdout.write('\n')

# This macro takes an arbitrary number of arguments, at least N = args.limit, and
# returns the N'th one. Looks like this:
#
# define VISIT_STRUCT_PP_ARG_N( \
#        _1, _2, _3, _4, _5, _6, _7, _8, _9,_10,  \
#        _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
#        _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
#        _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
#        _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
#        _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
#        _61,_62,_63,_64,_65,_66,_67,_68,_69,N,...) N

def write_pp_arg_n():
  write("#define VISIT_STRUCT_PP_ARG_N( ")

  x = 0
  n = 1
  while n <= args.limit:
    if x == 0:
      write("\\\n       ")
      x = 10
    x = x - 1
    
    write(" _")
    write(n)
    write(",")
    n = n + 1

  writeln(" N, ...) N")

# This macro returns the number of arguments, at most args.limit - 1, that it was passed.
# Looks like this.
# define VISIT_STRUCT_PP_NARG(...) VISIT_STRUCT_EXPAND(VISIT_STRUCT_PP_ARG_N(__VA_ARGS__, \
#        69,68,67,66,65,64,63,62,61,60, \
#        59,58,57,56,55,54,53,52,51,50, \
#        49,48,47,46,45,44,43,42,41,40, \
#        39,38,37,36,35,34,33,32,31,30, \
#        29,28,27,26,25,24,23,22,21,20, \
#        19,18,17,16,15,14,13,12,11,10, \
#        9,8,7,6,5,4,3,2,1,0))

def write_pp_narg():
  write("#define VISIT_STRUCT_PP_NARG(...) VISIT_STRUCT_EXPAND(VISIT_STRUCT_PP_ARG_N(__VA_ARGS__,")

  x = 0
  n = args.limit
  while n > 0:
    if x == 0:
      write("  \\\n       ")
      x = 10
    x = x - 1
    
    write(" ")
    write(n)
    write(",")
    n = n - 1

  writeln(" 0))")

# This macro applies the first argument, a macro, to each of the n arguments that follow.
# Looks like this.
# define VISIT_STRUCT_APPLYF3(f,_1,_2,_3) f(_1) f(_2) f(_3)
def write_pp_apply_f(n):
  write("#define VISIT_STRUCT_APPLYF")
  write(n)
  write("(f")
  
  for x in range(1, n + 1):
    write(",_")
    write(x)

  write(")")

  for x in range(1, n + 1):
    write(" f(_")
    write(x)
    write(")")

  writeln()

#
# Main routine
#

writeln("/*** Generated code ***/")
writeln()
writeln("static VISIT_STRUCT_CONSTEXPR const int max_visitable_members = " + str(args.limit) + ";")

writeln()
writeln("#define VISIT_STRUCT_EXPAND(x) x")
write_pp_arg_n()
write_pp_narg()

writeln()
writeln("/* need extra level to force extra eval */")
writeln("#define VISIT_STRUCT_CONCAT_(a,b) a ## b")
writeln("#define VISIT_STRUCT_CONCAT(a,b) VISIT_STRUCT_CONCAT_(a,b)")
writeln()

for n in range(args.limit + 1):
  write_pp_apply_f(n)

writeln()
writeln("#define VISIT_STRUCT_APPLY_F_(M, ...) VISIT_STRUCT_EXPAND(M(__VA_ARGS__))")
writeln("#define VISIT_STRUCT_PP_MAP(f, ...) VISIT_STRUCT_EXPAND(VISIT_STRUCT_APPLY_F_(VISIT_STRUCT_CONCAT(VISIT_STRUCT_APPLYF, VISIT_STRUCT_PP_NARG(__VA_ARGS__)), f, __VA_ARGS__))")

writeln()
writeln("/*** End generated code ***/")
