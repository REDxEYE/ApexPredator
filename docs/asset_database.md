# Asset database hashes

Both `kv` and `files` store `lookup3` (32-bit lookup3/hashlittle, seed zero) and
`murmur` (the first 64-bit output of MurmurHash3_x64_128, seed zero). Generation
Zero writes zero to `murmur`; Rage 2 computes both. `string_hashes(string_view)`
is the shared game-specific implementation used by the collectors and ADF string
storage.

```sql
CREATE TABLE kv (
    lookup3 INTEGER NOT NULL,
    murmur INTEGER NOT NULL DEFAULT 0,
    v TEXT NOT NULL,
    PRIMARY KEY (lookup3, murmur)
) WITHOUT ROWID;
CREATE TABLE files (
    lookup3 INTEGER NOT NULL,
    murmur INTEGER NOT NULL DEFAULT 0,
    name TEXT,
    size INTEGER NOT NULL,
    parent INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY (lookup3, murmur)
) WITHOUT ROWID;
```

The primary keys index lookup3; partial indexes index nonzero Murmur values.
The files parent index is retained. Distinct hash pairs are retained even when
lookup3 collides. Lookup by a single colliding hash returns the first pair in
numeric SQLite order; deletion by a hash removes all matching pairs. Zero Murmur
means unused and is excluded from queries/deletes. SQLite stores 64-bit hashes
as signed INTEGER bit patterns; convert negative results to unsigned when using
external SQL tools. Parent remains the containing archive's game-specific hash.

`kv_put(StringHashes, value)` and `files_put(StringHashes, name, size, parent)`
require both hash fields. Read/delete APIs accept `AssetDB::HashType::Lookup3`,
`Murmur`, or the default `Game` (lookup3 for GenZ, Murmur for Rage 2). RTPC name
resolution explicitly uses lookup3 through `find_lookup3_name`.

Opening an old `kv(k,v)` / `files(hash,name,size,parent)` database migrates both
tables in one transaction. Existing game keys, names, sizes, and parents are
preserved; Rage 2 lookup3 hashes are computed from stored strings, while GenZ
Murmur values remain zero. For unnamed Rage 2
files, lookup3 is zero because no string is available. A failure rolls back the
migration. Use the matching game's build when opening a legacy database; its
old schema does not identify the hash algorithm. Existing user databases are
not rewritten by the build itself; migration runs when an application opens one.

Collectors use `../hashes.db` for GenZ, `../rage2_hashes.db` for Rage 2, and
`../second_extinction_hashes.db` for Second Extinction, relative to their
working directory. All collector string/file insertion paths supply both hashes.
GenZ archive enumeration now visits TAB 2.1 entries.

## GTOC/STOC ingestion

Both collectors recognize `GT0C` by content, including files whose outer archive
path is still unknown. `.gtoc` global indexes and `.stoc` per-archive sidecars use
the same reader, `GTOCFile` in `include/apex/gtoc.h`.

The little-endian layout is:

- Header: `GT0C`, followed by a 32-bit archive count.
- Archive: 32-bit path hash, validation tag, member count, then member references.
- Member: 32-bit metadata displacement and payload offset. The displacement is
  unsigned and relative to its own field. Payload offset zero means an external
  resource, not an absent member.
- Metadata: 32-bit path hash, extension hash, byte size, then a NUL-terminated
  path. Metadata starts are 4-byte aligned and may be shared across members.

The reader exposes archives, their member offsets/file indices, and unique
referenced metadata records. It owns the bytes backing name views, supports moves
but not copies, and rejects invalid counts, references, and unterminated names.
Views remain valid while their owning reader is alive.

Collection inserts TOC member paths into `kv` with their game-specific
`string_hashes`. External members (offset zero) also receive `files` rows
with their recorded size and parent `0`. Embedded members remain catalog
names until the containing archive is available; their data is read through
the GTOC at its recorded offset. Stored TOC member and archive IDs are
lookup3 hashes, not TAB Murmur keys. Member enumeration uses the game's
asset-path hash; reading an embedded member requires a `files` mapping from
the container's lookup3 ID to its TAB Murmur key.

Tests cover both game configurations, legacy migration/reopening, hash
collisions, unsigned 64-bit keys/parents, zero Murmur handling, malformed TOC
boundaries, shared metadata, and running both collector executables against
synthetic RTPC/GTOC/STOC archives:

```sh
cmake --build cmake-build-debug --target GenerationZeroHashCollector Rage2HashCollector ApexGtocTests
ctest --test-dir cmake-build-debug -R '^Apex\.(Gtoc|HashCollectors)$' --output-on-failure
```

## Second Extinction location containers

`SecondExtinctionHashCollector` reads the RTPC `locations/world.bin` from the
mounted TABs before traversing GTOCs. Each location's `file` property is an
archive path stem. Location types 1 and 2 use `.bl`; type 0 uses `.nl` and/or
`.fl` according to its extension-list property (lookup3 `0xA64E1E84`).
Only paths present in mounted TABs are inserted into `kv` and `files`, with
both the lookup3 archive ID and Murmur TAB key, plus the TAB entry's size and
parent.

The collector parses `sarc.0.gtoc` to retain archive tag-to-lookup3 mappings
but defers its member traversal until after visiting TAB entries. A matching
container header tag registers its lookup3-to-Murmur mapping even when the
container has no `world.bin` name. The collector mounts the GTOC for ordinary
resource lookups, but walks its saved metadata directly: it resolves and
decompresses each available parent container once, then visits its embedded
(positive-offset) members as borrowed buffer views. External (offset-zero)
members are not read from that container. Routing every member through
`ArchiveManager::get()` instead would rescan the GTOC and decompress the same
parent for every child. Parent mappings must therefore be registered by the
TAB pass before either GTOC traversal or on-demand `GTOCArchive::get()` calls.
Tag-only registration leaves `files.name` empty unless a separately verified
path matches its TAB key: the tag supplies the lookup3-to-Murmur mapping, not
the container path.

The GTOC stores container hashes, tags, and member names, but not container
names. For Second Extinction, a positive-offset `.epe_adf` or `.epe` member
can identify an `.ee` container: replace the member extension with `.ee` and
accept the result only if its lookup3 equals the enclosing archive's ID.
Validated names go into `kv` even when their containers are not installed;
`files` receives a name only when the candidate's Murmur key occurs in a
mounted TAB. An offset-zero member is external and cannot establish its
container's name. A missing physical container remains reported with its
catalog path when one was recovered.

For an AVTX texture with a known name, the collector probes `.atx0` through
`.atx9` stream slices and records each mounted slice's parent. A TAB-owned
slice has parent `0`; an archive-owned slice uses that archive's key.
`ArchiveManager::get_parent_key_for()` returns a reference, so the TAB root
parent must be stored in stable storage rather than returned as a temporary.
The Second Extinction collector fixture checks a TAB-owned `.atx2` on two
successive scans in both Debug and Release.

`Apex.SecondExtinctionLocations` covers declared `.bl`, `.nl`, and `.fl`
containers, inferred `.ee` names with and without physical TAB entries,
unmatched/external member boundaries, tag-only mappings, auto-mounted GTOCs,
multiple embedded members at distinct offsets, and nested member reads.
