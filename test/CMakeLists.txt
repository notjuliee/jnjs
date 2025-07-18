add_executable(jnjs_tests
        basic.cpp
        functions.cpp
        class_binding.cpp
        function_binding.cpp
        module.cpp
)
target_link_libraries(jnjs_tests PRIVATE Catch2::Catch2WithMain jnjs)
catch_discover_tests(jnjs_tests)

add_executable(jnjs_benchmarks
        benchmarks.cpp
)
target_link_libraries(jnjs_benchmarks PRIVATE Catch2::Catch2WithMain jnjs)
