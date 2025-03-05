cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(visit_struct)

set(CMAKE_BUILD_TYPE Release)

file(GLOB headers ${PROJECT_SOURCE_DIR}/include/visit_struct/*.hpp)

install(FILES ${headers} DESTINATION include/visit_struct)
