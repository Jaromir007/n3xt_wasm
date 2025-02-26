cmake_minimum_required(VERSION 3.10)
project(Slicer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER emcc)

add_executable(slicer Slicer.cpp)

target_include_directories(slicer PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_link_options(
    -s MODULARIZE=1
    -s EXPORT_ES6=1
    -s ENVIRONMENT=web
    -s ALLOW_MEMORY_GROWTH=1
    -s EXPORTED_FUNCTIONS='["_slice", "_parseSTL", "_malloc", "_free"]'
    -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "getValue", "setValue", "HEAPU8", "HEAP32", "HEAPF32"]'
)

set_target_properties(slicer PROPERTIES 
    OUTPUT_NAME "slicer"
    SUFFIX ".js"
)
