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

add_executable(test-request test-request.c)

add_executable(test-client test-client.c)

add_executable(test-tcp-socket test-tcp-socket.c)

add_executable(test-file-as-socket test-file-as-socket.c)
