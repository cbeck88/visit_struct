#!/bin/bash

set -e

if [[ "$CXX" == "" ]]; then
  CXX="g++";
fi

${CXX} --version

if [[ "$BOOST_INCLUDE_DIR" == "" ]]; then
  BOOST_INCLUDE_DIR="/usr/include";
fi

echo "Boost Dir: " ${BOOST_INCLUDE_DIR}

${CXX} $@ -std=c++14 -I./include -isystem${BOOST_INCLUDE_DIR} -Wall -Werror -Wextra -pedantic test_visit_struct_boost_hana.cpp
./a.out
