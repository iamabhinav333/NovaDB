add_executable(novadb_tests
    test_disk_and_page.cpp
)

target_link_libraries(novadb_tests PRIVATE novadb)

add_test(NAME novadb_tests COMMAND novadb_tests)

add_executable(novadb_levels_6_8_tests
    test_levels_6_8.cpp
)

target_link_libraries(novadb_levels_6_8_tests PRIVATE novadb)

add_test(NAME novadb_levels_6_8_tests COMMAND novadb_levels_6_8_tests)

add_executable(novadb_levels_9_11_tests
    test_levels_9_11.cpp
)

target_link_libraries(novadb_levels_9_11_tests PRIVATE novadb)

target_compile_features(novadb_levels_9_11_tests PRIVATE cxx_std_20)

add_test(NAME novadb_levels_9_11_tests COMMAND novadb_levels_9_11_tests)
