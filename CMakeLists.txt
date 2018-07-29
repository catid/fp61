cmake_minimum_required(VERSION 3.5)
project(fp61)

set(CMAKE_CXX_STANDARD 11)

# Fp61 library source files
set(FP61_LIB_SRCFILES
        fp61.cpp
        fp61.h)

add_library(fp61 ${FP61_LIB_SRCFILES})

add_executable(tests tests/tests.cpp)
target_link_libraries(tests fp61)
