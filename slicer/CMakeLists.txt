cmake_minimum_required(VERSION 3.10)
project(Slicer)

set(CMAKE_CXX_STANDARD 17)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".wasm.js")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORT_ALL=1 -s EXPORTED_FUNCTIONS=['_malloc','_free','_slice'] -s MODULARIZE=1 -s ENVIRONMENT=web")
endif()


add_executable(slicer Main.cpp Slicer.cpp ParseSTL.cpp ToJSON.cpp)
