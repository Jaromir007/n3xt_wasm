cmake_minimum_required(VERSION 3.10)
project(MySlicer)

set(CMAKE_CXX_STANDARD 17)

add_executable(slicer Main.cpp Slicer.cpp ParseSTL.cpp)
