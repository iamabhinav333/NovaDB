add_executable(novadb_bench_disk
    bench_disk_io.cpp
)

target_link_libraries(novadb_bench_disk PRIVATE novadb)
