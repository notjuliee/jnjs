add_executable(jnjs_tests
        example_basic.cpp
        class_binding_tests.cpp
        runtime_context_tests.cpp
        script_eval_tests.cpp
        value_semantics_tests.cpp
        function_binding_tests.cpp
)
target_link_libraries(jnjs_tests PRIVATE Catch2::Catch2WithMain jnjs)
catch_discover_tests(jnjs_tests)
