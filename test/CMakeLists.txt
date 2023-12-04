include_directories(3rd)

link_libraries(echo)

add_executable(testing
    test.c
    basic_header.c
    basic_request.c
)

add_custom_target(run_testing
    DEPENDS testing
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/testing
    COMMENT "Testing..."
)

add_custom_target(test
    DEPENDS run_testing
)
