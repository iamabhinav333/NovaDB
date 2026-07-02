# NovaDB Architecture (Levels 0-2)

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
