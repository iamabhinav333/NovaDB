# NovaDB Architecture (Levels 0-8)

## Level 0 - Foundation
- Build system: CMake with a reusable static library (`novadb`)
- Project layout: `src/`, `include/`, `tests/`, `docs/`, `benchmarks/`
- Shared utilities:
  - Logger (`novadb::Logger`)
  - Config (`novadb::Config`, plus env-based override)

## Level 1 - Disk Manager
`novadb::DiskManager` owns file-backed byte storage and exposes:
- `read(offset, size)`
- `write(offset, data)`
- `read_page(page_id, page_size)`
- `write_page(page_id, page_data, page_size)`

The file is auto-created at startup if it does not exist (default: `database.db`).

## Why Databases Use Pages
Databases use fixed-size pages (e.g., 4096 bytes) because:
- Storage hardware and OS caches are optimized around blocks/pages.
- Fixed units simplify buffer management and replacement policies.
- Predictable I/O enables better performance and easier crash recovery.
- Metadata and record layouts are easier to reason about in page frames.

## Level 2 - Page Manager
Buffer layer components:
- `Page`: in-memory page object with `page_id`, `dirty`, `pin_count`, and byte array.
- `IPageIO`: abstraction for page I/O (decouples buffer logic from disk implementation).
- `DiskPageIO`: adapter over `DiskManager`.
- `PageManager`: create/fetch/flush pages independent from raw file details.

This split keeps page logic testable and allows replacing disk I/O with mocks or alternate backends later.

## Level 3 - Record Storage
Record storage uses a slotted-page format inside fixed-size pages.

- Record format:
  - key (uint64)
  - value length (uint64)
  - value bytes
- Slotted-page layout:
  - header stores slot count, free-space start, free-space end
  - slot directory grows from front
  - record payload grows from back

This enables in-page free-space tracking and supports insert/delete/update/search operations per page.

## Level 4 - Table Manager
Multiple table support is provided by two components:

- `Catalog`:
  - persistent metadata file (`catalog.meta`)
  - stores table name, table file path, and next page id
- `TableManager`:
  - create/open/list/delete table APIs
  - table files are stored separately under a data directory

This metadata layer is the beginning of a system catalog in a relational engine.

## Level 5 - Buffer Pool
`BufferPoolManager` provides in-memory caching to reduce disk I/O.

- Caches pages up to a fixed pool size
- Tracks pin counts so active pages are not evicted
- Uses LRU replacement over unpinned pages
- Flushes dirty pages on demand and during eviction

This improves read/write locality and provides the foundation for higher-level transaction and concurrency work.

## Level 6 - B+ Tree Index
The index layer uses an in-memory B+ tree with an explicit node structure.

- `Node` tracks leaf/internal state, keys, values, children, and leaf links
- Inserts and deletes mutate a sorted entry map and rebuild the tree structure
- Search follows internal separators down to a leaf
- Range queries walk the linked leaf chain from a start key to an end key

This implementation is intentionally educational: it keeps the node topology visible while avoiding a large amount of balancing code in the first pass.

## Level 7 - Query Layer
The query layer turns simple text commands into index operations.

- `QueryParser` tokenizes commands and extracts table/key/value arguments
- `QueryEngine` dispatches requests to the appropriate B+ tree for a table
- Supported commands include insert, find, update, delete, range, begin, commit, and rollback

This layer is the first user-facing execution surface of the database.

## Level 8 - Transactions
Transactions are tracked with a small state machine.

- `TransactionManager` tracks idle, active, committed, and rolled-back states
- `QueryEngine` snapshots table indexes on `BEGIN`
- `ROLLBACK` restores the snapshot
- `COMMIT` keeps the current state and records the terminal transaction state

This gives the query layer atomicity for the current in-memory implementation.
