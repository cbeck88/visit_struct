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

${CXX} $@ -std=c++11 -I./include -Wall -Werror -Wextra -pedantic test_visit_struct.cpp
./a.out

${CXX} $@ -std=c++11 -I./include -isystem${BOOST_INCLUDE_DIR} -Wall -Werror -Wextra -pedantic test_visit_struct_boost_fusion.cpp
./a.out

${CXX} $@ -std=c++11 -I./include -isystem${BOOST_INCLUDE_DIR} -Wall -Werror -Wextra -pedantic test_visit_struct_intrusive.cpp
./a.out

if [[ "$NO_CXX14" == "" ]]; then

${CXX} $@ -std=c++14 -I./include -Wall -Werror -Wextra -pedantic test_visit_struct.cpp;
./a.out;

${CXX} $@ -std=c++14 -I./include -isystem${BOOST_INCLUDE_DIR} -Wall -Werror -Wextra -pedantic test_visit_struct_boost_fusion.cpp;
./a.out;

${CXX} $@ -std=c++14 -I./include -isystem${BOOST_INCLUDE_DIR} -Wall -Werror -Wextra -pedantic test_visit_struct_intrusive.cpp
./a.out

fi
