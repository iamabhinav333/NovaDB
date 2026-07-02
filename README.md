# NovaDB

NovaDB is a learning-focused database engine project in C++.
This stage implements **Level 0 (Foundation)**, **Level 1 (Disk Manager)**, and **Level 2 (Page Manager)**.

## Project Structure

- `src/`: implementation files
- `include/`: public headers
- `tests/`: basic test executables
- `docs/`: design and architecture notes
- `benchmarks/`: basic performance probes

## Implemented Levels

### Level 0 - Foundation
- Clean folder layout
- CMake project configuration
- Logging utility (`novadb::Logger`)
- Configuration system (`novadb::Config`)
  - Fields: page size, db file path, cache size
  - Optional environment overrides:
    - `NOVADB_PAGE_SIZE`
    - `NOVADB_DB_FILE`
    - `NOVADB_CACHE_PAGES`

### Level 1 - Disk Manager
- Auto-creates database file (`database.db`) if missing
- Byte-oriented APIs:
  - `read(offset, size)`
  - `write(offset, data)`
- Fixed page APIs (default 4096 bytes):
  - `read_page(page_id, page_size)`
  - `write_page(page_id, page_data, page_size)`

### Level 2 - Page Manager
- `Page` abstraction with:
  - `page_id`
  - dirty flag
  - pin count
  - page data buffer
- `PageManager` operations:
  - create page
  - fetch page
  - modify page data
  - flush page(s)
- `IPageIO` abstraction keeps page layer independent from disk details

## Build

```powershell
cmake -S . -B build
cmake --build build
```

## Run demo

```powershell
.\build\novadb_demo.exe
```

## Run tests

```powershell
ctest --test-dir build --output-on-failure
```

## Notes

See `docs/architecture.md` for details about the design and why fixed-size pages are central to database engines.
