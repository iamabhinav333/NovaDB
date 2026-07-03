# NovaDB

NovaDB is a learning-focused database engine project in C++.
This stage implements **Level 0 (Foundation)** through **Level 11 (Query Optimization)**.

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

### Level 3 - Record Storage
- Structured record serialization:
  - `Record { key, value }`
  - `serialize_record` and `deserialize_record`
- Slotted-page record storage (`RecordPage`) with:
  - insert
  - delete
  - update
  - search by key
  - free-space accounting inside a fixed page

### Level 4 - Table Manager
- Multiple-table support through `TableManager`
- Metadata catalog persisted by `Catalog` (`catalog.meta`)
- Table operations:
  - create
  - open
  - list
  - delete

### Level 5 - Buffer Pool
- `BufferPoolManager` caches frequently used pages in memory
- LRU page replacement for unpinned pages
- Dirty-page flushing:
  - `flush_page`
  - `flush_all`
  - flush-on-eviction when needed

### Level 6 - B+ Tree Index
- In-memory B+ tree index with explicit node structure
- Insert, search, delete, and range queries
- Node split and merge behavior preserved by tree rebuilds after mutations

### Level 7 - Query Layer
- Simple command parser and dispatcher
- Supported commands:
  - `INSERT table key value`
  - `FIND table key`
  - `UPDATE table key value`
  - `DELETE table key`
  - `RANGE table start end`
- Transaction commands:
  - `BEGIN`
  - `COMMIT`
  - `ROLLBACK`

### Level 8 - Transactions
- Transaction state tracking via `TransactionManager`
- Snapshot-based rollback in the query layer
- Commit/rollback states are visible after completion

### Level 9 - Write Ahead Logging
- Append log records before applying data changes
- Replay log files during restart
- Recover incomplete transactions via WAL-backed replay

### Level 10 - Concurrency
- Shared and exclusive lock modes
- Timeout-based lock manager
- Basic contention handling for multiple users

### Level 11 - Query Optimization
- Simple plan selection based on table statistics
- Prefer indexes for point lookups and narrow ranges
- Fall back to scans when no index is present or ranges are wide

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

See `docs/architecture.md` for architecture details for Levels 0 through 11.
