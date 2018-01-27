
set(SOURCES
    main.c
    test_header.c
)

add_executable(uvhttp_test ${SOURCES})
target_link_libraries(uvhttp_test uvhttp)