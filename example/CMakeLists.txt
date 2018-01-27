
set(SOURCES
    main.c
)

add_executable(req ${SOURCES})
target_link_libraries(req uvhttp)