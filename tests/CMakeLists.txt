add_executable(novadb_tests
    test_disk_and_page.cpp
)

target_link_libraries(novadb_tests PRIVATE novadb)

add_test(NAME novadb_tests COMMAND novadb_tests)
