add_executable(jnjs_tests
        basic.cpp
        functions.cpp
        class_binding.cpp
)
target_link_libraries(jnjs_tests PRIVATE Catch2::Catch2WithMain jnjs)
catch_discover_tests(jnjs_tests)

add_executable(jnjs_benchmarks
        benchmarks.cpp
        common.h
)
target_link_libraries(jnjs_benchmarks PRIVATE Catch2::Catch2WithMain jnjs)
