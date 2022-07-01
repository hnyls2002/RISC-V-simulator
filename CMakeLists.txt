cmake_minimum_required(VERSION 3.16)
project(RISC_V_simulator)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-O2")

add_executable(code main.cpp Components.hpp Exception.hpp Simulator.hpp Decode.hpp)
