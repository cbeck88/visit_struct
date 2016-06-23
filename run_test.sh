#!/bin/bash

set -e

if [[ "$BOOST_INCLUDE_DIR" == "" ]]; then
  BOOST_INCLUDE_DIR="/usr/include";
fi

echo ${BOOST_INCLUDE_DIR}

${CXX} $@ -std=c++11 -I. -Wall -Werror -Wextra -pedantic test_visit_struct.cpp
./a.out

${CXX} $@ -std=c++11 -I. -isystem${BOOST_INCLUDE_DIR} -Wall -Werror -Wextra -pedantic test_visit_struct_boost_fusion.cpp
./a.out

if [[ "$NO_CXX14" == "" ]]; then

${CXX} $@ -std=c++14 -I. -Wall -Werror -Wextra -pedantic test_visit_struct.cpp;
./a.out;

${CXX} $@ -std=c++14 -I. -isystem${BOOST_INCLUDE_DIR} -Wall -Werror -Wextra -pedantic test_visit_struct_boost_fusion.cpp;
./a.out;

fi
