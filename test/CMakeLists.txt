include_directories(3rd)

link_libraries(echo)

add_executable(basic_header
    test.c
    basic_header.c
)

add_custom_target(basic_header_test
    DEPENDS basic_header
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/basic_header
    COMMENT "Testing basic header operations..."
)

add_custom_target(test
    DEPENDS basic_header_test
)

add_executable(test-request test-request.c)

add_executable(test-client test-client.c)

add_executable(test-tcp-socket test-tcp-socket.c)

add_executable(test-file-as-socket test-file-as-socket.c)
