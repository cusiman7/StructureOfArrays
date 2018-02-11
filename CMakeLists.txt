cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_FLAGS "-std=c++1z -stdlib=libc++ -fno-exceptions -fno-rtti -Wall -Wextra")

project (SoA)

add_executable(soa
                main.cpp
                SoA.h
)

