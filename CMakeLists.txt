cmake_minimum_required(VERSION 3.14)
project(jnjs VERSION 0.1.0 LANGUAGES CXX)

if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "Do not build in the source directory please it makes a mess :(")
endif ()

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(JNJS_ROOT_PROJECT ON)
else ()
    set(JNJS_ROOT_PROJECT OFF)
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CMakeDependentOption)

option(JNJS_ENABLE_TESTING "Enable testing" ${JNJS_ROOT_PROJECT})
cmake_dependent_option(JNJS_USE_EXTERNAL_CATCH2 "Use installed catch2 instead of submodule" OFF JNJS_ENABLE_TESTING OFF)
option(USE_EXTERNAL_QUICKJS "Use installed quickjs instead of submodule" OFF)

if (JNJS_ENABLE_TESTING)
    set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
    enable_testing()
endif ()

add_subdirectory(ext EXCLUDE_FROM_ALL)

add_library(jnjs
        src/binding.cpp
        src/context.cpp
        src/function.cpp
        src/function_helpers.cpp
        src/runtime.cpp
        src/value.cpp
        src/value_helpers.cpp
)
target_include_directories(jnjs PUBLIC include PRIVATE src)
target_link_libraries(jnjs PRIVATE qjs::qjs)

add_executable(jnjs_cli main.cpp)
target_link_libraries(jnjs_cli jnjs)

if (JNJS_ENABLE_TESTING)
    add_subdirectory(test)
endif ()